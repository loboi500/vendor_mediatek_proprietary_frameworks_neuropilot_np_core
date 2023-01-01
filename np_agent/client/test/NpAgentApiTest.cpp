/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "NpAgentApiTest"

#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <gtest/gtest.h>
#include <poll.h>
#include <ui/gralloc_extra.h>
#include <utils/Errors.h>
#include <vndk/hardware_buffer.h>

#include <fstream>
#include <string>
#include <thread>

#include "NpAgentApiTestCache.h"
#include "NpAgentApiTestModel.h"
#include "NpAgentShim.h"
#include "Profiler.h"

using android::status_t;
using testing::Test;

static constexpr uint32_t kBatch = 1;
static constexpr uint32_t kInputH = 1600;        // Input height
static constexpr uint32_t kInputW = 720;         // Input width
static constexpr uint32_t kInputStride = 736;    // Input stride
static constexpr uint32_t kInputC = 4;           // Input channel
static constexpr uint32_t kOutputH = 2400;       // Output height
static constexpr uint32_t kOutputW = 1080;       // Output width
static constexpr uint32_t kOutputStride = 1088;  // Output stride
static constexpr uint32_t kOutputC = 4;          // Output channel
static constexpr uint32_t kInputSize = (kBatch * kInputH * kInputStride * kInputC);
static constexpr uint32_t kOutputSize = (kBatch * kOutputH * kOutputStride * kOutputC);
static constexpr uint32_t kBoostValue = 90;

class NpAgentApiTest : public Test {
protected:
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

protected:
    virtual void SetUp() override {
        const ::testing::TestInfo* const testInfo =
            ::testing::UnitTest::GetInstance()->current_test_info();
        mTestName = mTestName + testInfo->test_case_name() + "_" + testInfo->name();
        LOG(INFO) << "BEGIN: " << mTestName;
    }

    virtual void TearDown() override {
#if !defined(NDEBUG)
        std::ostringstream output;
        profiler::NpProfiler::GetInstance().Print(output, 20);
        LOG(INFO) << output.str();
#endif
        LOG(INFO) << "END: " << mTestName;
    }

private:
    std::string mTestName;
};

void pollingFenceFd(int fenceFd) {
    LOG(INFO) << "Start of polling fence fd: " << fenceFd;
    struct pollfd fds;
    fds.fd = fenceFd;
    fds.events = POLLIN;
    int pollRet;
    do {
        pollRet = poll(&fds, 1, 30000);
        if (pollRet > 0) {
            if (fds.revents & POLLNVAL) {
                errno = EINVAL;
                break;
            }
            if (fds.revents & POLLERR) {
                errno = EINVAL;
                break;
            }
            break;
        } else if (pollRet == 0) {
            errno = ETIME;
            break;
        }
    } while (pollRet == -1 && (errno == EINTR || errno == EAGAIN));
    LOG(INFO) << "End of polling fence fd: " << fenceFd;
}

int32_t preparedModelFromDlaPath() {
    // 1. Read DLA into AHWB
    // 2. Get fd from the AWHB
    // 3. Create NP Agent from the fd
    const std::string dlaPath = "/data/local/tmp/NpAgentApiTest.dla";
    std::ifstream is(dlaPath, std::ios::in | std::ios::binary | std::ios::ate);
    if (!is.is_open()) {
        LOG(ERROR) << "Fail to open " << dlaPath;
        return -1;
    }

    const size_t size = is.tellg();
    LOG(INFO) << "Input dla size: " << size;
    is.seekg(0, std::ios::beg);

    AHardwareBuffer_Desc desc{
        .width = static_cast<uint32_t>(size),
        .height = 1,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_BLOB,
        .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
    };

    AHardwareBuffer* ahwb = nullptr;
    void* buffer = nullptr;
    auto ahwbGuard = android::base::make_scope_guard([ahwb, buffer]() {
        if (ahwb != nullptr) {
            if (buffer != nullptr) {
                EXPECT_EQ(AHardwareBuffer_unlock(ahwb, nullptr), 0);
            }
            AHardwareBuffer_release(ahwb);
        }
    });

    EXPECT_EQ(AHardwareBuffer_allocate(&desc, &ahwb), 0);
    if (ahwb == nullptr) {
        return -1;
    }

    EXPECT_EQ(AHardwareBuffer_lock(ahwb, desc.usage, -1, NULL, &buffer), 0);
    if (buffer == nullptr) {
        return -1;
    }
    is.read(reinterpret_cast<char*>(buffer), size);

    LOG(INFO) << "Start to prepare model from cache fd, size: " << size;
    int fd = -1;
    int err =
        ::gralloc_extra_query(AHardwareBuffer_getNativeHandle(ahwb), GRALLOC_EXTRA_GET_ION_FD, &fd);
    EXPECT_EQ(err, GRALLOC_EXTRA_OK);

    // Prepare options for loading model cache
    NpAgentOptions* options = nullptr;
    LOG(INFO) << "Set model options";
    EXPECT_EQ(NpAgentOptions_create(&options), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setInputFormat(options, HAL_PIXEL_FORMAT_RGBA_8888), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setOutputFormat(options, HAL_PIXEL_FORMAT_RGB_888), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setInputCompressionMode(options, COMPRESSION_NONE), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setOutputCompressionMode(options, COMPRESSION_NONE), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setInputHeightWidth(options, kInputH, kInputW), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setOutputHeightWidth(options, kOutputH, kOutputW), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setInputStride(options, kInputStride), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setOutputStride(options, kOutputStride), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentOptions_setBoostValue(options, kBoostValue), RESULT_NO_ERROR);

    int32_t agentId = NpAgent_createFromCacheFd(fd, size, options);
    LOG(INFO) << "Prepare model from cache fd done";
    EXPECT_GT(agentId, 0);

    NpAgentOptions_release(options);

    return agentId;
}

bool compute(int32_t agentId, bool applyFence = false) {
    const std::string inputPath = "/data/local/tmp/NpAgentApiTestInput.bin";
    const std::string outputPath = "/data/local/tmp/NpAgentApiTestOutput.bin";

    std::ifstream is(inputPath, std::ios::in | std::ios::binary | std::ios::ate);
    auto inputSize = is.tellg();
    LOG(INFO) << "Input bin size: " << inputSize;
    is.seekg(0, std::ios::beg);

    std::ofstream os(outputPath, std::ios::out | std::ios::binary);

    if (!is.is_open() || !os.is_open()) {
        LOG(ERROR) << "Fail to open " << inputPath << " or " << outputPath;
        is.close();
        os.close();
        return false;
    }

    AHardwareBuffer* ahwbInput = nullptr;
    AHardwareBuffer* ahwbOutput = nullptr;
    void* inputBuffer = nullptr;
    void* outputBuffer = nullptr;

    auto ahwbGuard =
        android::base::make_scope_guard([ahwbInput, inputBuffer, ahwbOutput, outputBuffer]() {
            if (ahwbInput != nullptr) {
                if (inputBuffer != nullptr) {
                    AHardwareBuffer_unlock(ahwbInput, nullptr);
                }
                AHardwareBuffer_release(ahwbInput);
            }
            if (ahwbOutput != nullptr) {
                if (outputBuffer != nullptr) {
                    AHardwareBuffer_unlock(ahwbOutput, nullptr);
                }
                AHardwareBuffer_release(ahwbOutput);
            }
        });

    AHardwareBuffer_Desc descInput{
        .width = static_cast<uint32_t>(inputSize),
        .height = 1,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_BLOB,
        .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
        .stride = static_cast<uint32_t>(inputSize),
    };

    AHardwareBuffer_Desc descOutput{
        .width = kOutputSize,
        .height = 1,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_BLOB,
        .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
        .stride = kOutputSize,
    };

    EXPECT_EQ(AHardwareBuffer_allocate(&descInput, &ahwbInput), android::NO_ERROR);
    EXPECT_EQ(AHardwareBuffer_allocate(&descOutput, &ahwbOutput), android::NO_ERROR);

    if (ahwbInput == nullptr || ahwbOutput == nullptr) {
        LOG(ERROR) << "Fail to allocate ahwbInput or ahwbOutput";
        return false;
    }

    // Copy input to ahwb
    EXPECT_EQ(
        AHardwareBuffer_lock(
            ahwbInput, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
            -1, nullptr, &inputBuffer),
        android::NO_ERROR);
    is.read(reinterpret_cast<char*>(inputBuffer), inputSize);
    is.close();

    int inputFd = -1;
    int outputFd = -1;
    int err = ::gralloc_extra_query(AHardwareBuffer_getNativeHandle(ahwbInput),
                                    GRALLOC_EXTRA_GET_ION_FD, &inputFd);
    EXPECT_EQ(err, GRALLOC_EXTRA_OK);
    err = ::gralloc_extra_query(AHardwareBuffer_getNativeHandle(ahwbOutput),
                                GRALLOC_EXTRA_GET_ION_FD, &outputFd);
    EXPECT_EQ(err, GRALLOC_EXTRA_OK);

    // Execution
    NpAgentExecution* execution = nullptr;
    EXPECT_EQ(NpAgentExecution_create(&execution), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentExecution_setInput(execution, inputFd, kInputSize), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentExecution_setOutput(execution, outputFd, kOutputSize), RESULT_NO_ERROR);
    if (!applyFence) {
        EXPECT_EQ(NpAgent_compute(agentId, execution), RESULT_NO_ERROR);
    } else {
        // Compute with fence
        // Because the test case cannot generate its own input fence, it is replaced by -1
        int releaseFence1 = 0;
        {
            PROFILE_ME_AS("NpAgent_computeWithFence-1");
            EXPECT_EQ(NpAgent_computeWithFence(agentId, execution, -1, -1, &releaseFence1, 0),
                      RESULT_NO_ERROR);
        }
        EXPECT_GT(releaseFence1, 0);

        // Because the test case cannot generate its own input fence, it is replaced by -1
        int releaseFence2 = 0;
        {
            PROFILE_ME_AS("NpAgent_computeWithFence-2");
            EXPECT_EQ(NpAgent_computeWithFence(agentId, execution, -1, -1, &releaseFence2, 0),
                      RESULT_NO_ERROR);
        }
        EXPECT_GT(releaseFence2, 0);

        // Use releaseFence1 & releaseFence2 as the new input & output fence and re-compute again
        int releaseFence3 = 0;
        {
            PROFILE_ME_AS("NpAgent_computeWithFence-3");
            EXPECT_EQ(NpAgent_computeWithFence(agentId, execution, releaseFence1, releaseFence2,
                                               &releaseFence3, 0),
                      RESULT_NO_ERROR);
        }
        EXPECT_GT(releaseFence3, 0);
        {
            PROFILE_ME_AS("poll-3");
            pollingFenceFd(releaseFence3);
        }
        close(releaseFence1);
        close(releaseFence2);
        close(releaseFence3);
    }

    // Save output buffer to file
    EXPECT_EQ(AHardwareBuffer_lock(
                  ahwbOutput,
                  AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1,
                  nullptr, &outputBuffer),
              android::NO_ERROR);
    if (outputBuffer != nullptr) {
        os.write(reinterpret_cast<char*>(outputBuffer), kOutputSize);
        os.close();
    }
    return true;
}

TEST_F(NpAgentApiTest, PreparedModelFromDlaPath) {
    LOG(INFO) << "Get attributes";
    int32_t agentId = preparedModelFromDlaPath();
    NpAgent_release(agentId);
}

TEST_F(NpAgentApiTest, GetAttributes) {
    LOG(INFO) << "Load DLA";
    int32_t agentId = preparedModelFromDlaPath();
    LOG(INFO) << "Get attributes";

    /* ===== Start of agent attributes validation ===== */
    // If you need to validate input data frequently, you can use NpAgentAttributes as a global
    // variable or a private member of a class to easily compare data.
    NpAgentAttributes* attributes = nullptr;
    uint32_t value;
    EXPECT_EQ(NpAgentAttributes_create(agentId, &attributes), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgentAttributes_getInputFormat(attributes, &value), RESULT_NO_ERROR);
    EXPECT_EQ(value, HAL_PIXEL_FORMAT_RGBA_8888);
    EXPECT_EQ(NpAgentAttributes_getOutputFormat(attributes, &value), RESULT_NO_ERROR);
    EXPECT_EQ(value, HAL_PIXEL_FORMAT_RGB_888);
    EXPECT_EQ(NpAgentAttributes_getInputCompressionMode(attributes, &value), RESULT_NO_ERROR);
    EXPECT_EQ(value, COMPRESSION_NONE);
    EXPECT_EQ(NpAgentAttributes_getOutputCompressionMode(attributes, &value), RESULT_NO_ERROR);
    EXPECT_EQ(value, COMPRESSION_NONE);
    EXPECT_EQ(NpAgentAttributes_getInputStride(attributes, &value), RESULT_NO_ERROR);
    EXPECT_EQ(value, kInputStride);
    EXPECT_EQ(NpAgentAttributes_getOutputStride(attributes, &value), RESULT_NO_ERROR);
    EXPECT_EQ(value, kOutputStride);
    NpAgentAttributes_release(attributes);
    /* ===== End of agent attributes validation ===== */

    NpAgent_release(agentId);
}

TEST_F(NpAgentApiTest, ValidateInputOutput) {
    LOG(INFO) << "Load DLA";
    int32_t agentId = preparedModelFromDlaPath();

    LOG(INFO) << "Validate input";
    EXPECT_EQ(
        NpAgent_validateInput(agentId, HAL_PIXEL_FORMAT_RGBA_8888, kInputH, kInputW, kInputStride),
        RESULT_NO_ERROR);
    LOG(INFO) << "Validate output";
    EXPECT_EQ(NpAgent_validateOutput(agentId, HAL_PIXEL_FORMAT_RGB_888, kOutputH, kOutputW,
                                     kOutputStride),
              RESULT_NO_ERROR);

    NpAgent_release(agentId);
}

TEST_F(NpAgentApiTest, Compute) {
    LOG(INFO) << "Load DLA";
    int32_t agentId = preparedModelFromDlaPath();
    LOG(INFO) << "Compute";
    compute(agentId);
    NpAgent_release(agentId);
}

TEST_F(NpAgentApiTest, ComputewithFence) {
    LOG(INFO) << "Load DLA";
    int32_t agentId = preparedModelFromDlaPath();
    LOG(INFO) << "Compute with fence";
    compute(agentId, true);
    NpAgent_release(agentId);
}

TEST_F(NpAgentApiTest, ThreadComputewithFence) {
    int32_t agentId1 = preparedModelFromDlaPath();
    LOG(INFO) << "Agent id for theread 1: " << agentId1;
    int32_t agentId2 = preparedModelFromDlaPath();
    LOG(INFO) << "Agent id for theread 2: " << agentId2;

    std::thread t1([agentId1]() {
        for (uint32_t i = 0; i < 5; i++) {
            compute(agentId1, true);
        }
    });

    std::thread t2([agentId2]() {
        for (uint32_t i = 0; i < 5; i++) {
            compute(agentId2, true);
        }
    });

    t1.join();
    t2.join();

    NpAgent_release(agentId1);
    NpAgent_release(agentId2);
}

TEST_F(NpAgentApiTest, UpdateBoostValue) {
    LOG(INFO) << "Load DLA";
    int32_t agentId = preparedModelFromDlaPath();

    NpAgentOptions* options = nullptr;
    EXPECT_EQ(NpAgentOptions_create(&options), RESULT_NO_ERROR);

    LOG(INFO) << "Update boost value to 10";
    EXPECT_EQ(NpAgentOptions_setBoostValue(options, 10), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgent_updateOptions(agentId, options, OPTION_BOOST_VALUE), RESULT_NO_ERROR);

    LOG(INFO) << "Update boost value to 20";
    EXPECT_EQ(NpAgentOptions_setBoostValue(options, 20), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgent_updateOptions(agentId, options, OPTION_BOOST_VALUE), RESULT_NO_ERROR);

    LOG(INFO) << "Update boost value to 50";
    EXPECT_EQ(NpAgentOptions_setBoostValue(options, 50), RESULT_NO_ERROR);
    EXPECT_EQ(NpAgent_updateOptions(agentId, options, OPTION_BOOST_VALUE), RESULT_NO_ERROR);

    compute(agentId, true);

    if (options != nullptr) {
        NpAgentOptions_release(options);
    }

    NpAgent_release(agentId);
}
