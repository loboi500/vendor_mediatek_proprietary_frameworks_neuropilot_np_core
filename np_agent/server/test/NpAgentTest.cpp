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

#define LOG_TAG "NpAgentTest"

#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <android/hardware_buffer.h>
#include <gtest/gtest.h>
#include <poll.h>
#include <ui/gralloc_extra.h>
#include <vndk/hardware_buffer.h>

#include <fstream>
#include <ostream>

#include "hal/1.1/IAgent_V1_1.h"

using testing::Test;
using vendor::mediatek::hardware::neuropilot::agent::V1_0::Options;
using vendor::mediatek::hardware::neuropilot::agent::V1_1::IAgent;

static constexpr uint32_t kInputN = 1;
static constexpr uint32_t kInputH = 1280;
static constexpr uint32_t kInputW = 720;
static constexpr uint32_t kInputC = 4;
static constexpr uint32_t kOutputN = 1;
static constexpr uint32_t kOutputH = 1920;
static constexpr uint32_t kOutputW = 1080;
static constexpr uint32_t kOutputC = 3;
static constexpr uint32_t kOutputSize = (kOutputN * kOutputH * kOutputW * kOutputC);

android::sp<IAgent> g_service;

class NpAgentTest : public Test {
protected:
    static void SetUpTestCase() {
        g_service = IAgent::getService();
        if (g_service == nullptr) {
            LOG(ERROR) << "faild to get IAgent test serivce";
        }
    }

    static void TearDownTestCase() { g_service = nullptr; }

protected:
    virtual void SetUp() override {
        const ::testing::TestInfo* const testInfo =
            ::testing::UnitTest::GetInstance()->current_test_info();
        mTestName = mTestName + testInfo->test_case_name() + "_" + testInfo->name();
        LOG(INFO) << "BEGIN: " << mTestName;
    }

    virtual void TearDown() override { LOG(INFO) << "END: " << mTestName; }

private:
    std::string mTestName;
};

void allocateAHardwareBuffer(AHardwareBuffer** ahwb, uint32_t size) {
    AHardwareBuffer_Desc desc{
        .width = size,
        .height = 1,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_BLOB,
        .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
        .stride = size,
    };

    EXPECT_EQ(AHardwareBuffer_allocate(&desc, ahwb), 0);
}

hidl_memory createHidlMemoryFromAHardwareBuffer(AHardwareBuffer* ahwb, uint32_t size) {
    int fd = -1;
    int err =
        ::gralloc_extra_query(AHardwareBuffer_getNativeHandle(ahwb), GRALLOC_EXTRA_GET_ION_FD, &fd);
    EXPECT_EQ(err, GRALLOC_EXTRA_OK);
    EXPECT_NE(fd, 0);

    int dupfd = dup(fd);
    EXPECT_NE(dupfd, 0);

    native_handle_t* nativeHandle = native_handle_create(1, 0);
    EXPECT_NE(nativeHandle, nullptr);

    nativeHandle->data[0] = dupfd;

    android::hardware::hidl_handle hidlHandle;
    hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
    return hidl_memory("ion_fd", std::move(hidlHandle), size);
}

void releaseHidlMemory(const hidl_memory& memory) {
    int fd = memory.handle()->data[0];
    close(fd);
}

uint32_t getPreparedModelSize(int32_t modelId) { return g_service->getPreparedModelSize(modelId); }

int32_t prepareModelFromDlaPath() {
    const std::string dlaPath = "/data/local/tmp/test.dla";
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

    hidl_memory hidlMemory = createHidlMemoryFromAHardwareBuffer(ahwb, static_cast<uint32_t>(size));
    auto hidlMemoryGuard =
        android::base::make_scope_guard([hidlMemory]() { releaseHidlMemory(hidlMemory); });

    Options options;
    int32_t modelId = g_service->prepareModelFromCache(hidlMemory, options);
    EXPECT_GE(modelId, 0);

    return modelId;
}

bool compute(int32_t modelId, bool executedFenced) {
    const std::string inputPath = "/data/local/tmp/input.bin";
    const std::string outputPath = "/data/local/tmp/output.bin";

    std::ifstream is(inputPath, std::ios::in | std::ios::binary | std::ios::ate);
    const size_t inputSize = is.tellg();
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
        .width = (kOutputN * kOutputH * kOutputW * kOutputC),
        .height = 1,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_BLOB,
        .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
        .stride = (kOutputN * kOutputH * kOutputW * kOutputC),
    };

    EXPECT_EQ(AHardwareBuffer_allocate(&descInput, &ahwbInput), 0);
    EXPECT_EQ(AHardwareBuffer_allocate(&descOutput, &ahwbOutput), 0);

    if (ahwbInput == nullptr || ahwbOutput == nullptr) {
        LOG(ERROR) << "Fail to allocate ahwbInput or ahwbOutput";
        return false;
    }

    // Copy input to ahwb
    EXPECT_EQ(
        AHardwareBuffer_lock(
            ahwbInput, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
            -1, nullptr, &inputBuffer),
        0);
    is.read(reinterpret_cast<char*>(inputBuffer), inputSize);

    hidl_memory input =
        createHidlMemoryFromAHardwareBuffer(ahwbInput, (kInputN * kInputH * kInputW * kInputC));
    hidl_memory output = createHidlMemoryFromAHardwareBuffer(
        ahwbOutput, (kOutputN * kOutputH * kOutputW * kOutputC));

    auto hidlMemoryGuard = android::base::make_scope_guard([input, output]() {
        releaseHidlMemory(input);
        releaseHidlMemory(output);
    });

    if (executedFenced) {
        ErrorStatus result;
        ErrorStatus executionStatus;
        hidl_handle syncFenceHandle;
        sp<IFencedExecutionCallback> fencedCallback = nullptr;
        auto callbackFunc = [&result, &syncFenceHandle, &fencedCallback](
                                ErrorStatus error, const hidl_handle& handle,
                                const sp<IFencedExecutionCallback>& callback) {
            result = error;
            syncFenceHandle = handle;
            fencedCallback = callback;
        };

        g_service->executeFenced(modelId, {input}, {output}, {}, callbackFunc);
        EXPECT_EQ(result, ErrorStatus::NONE);
        EXPECT_NE(fencedCallback, nullptr);
        LOG(INFO) << "Got output fence fd: " << syncFenceHandle.getNativeHandle()->data[0];
        EXPECT_GT(syncFenceHandle.getNativeHandle()->data[0], 0);
        int syncFenceFd = dup(syncFenceHandle.getNativeHandle()->data[0]);
        LOG(INFO) << "Start of polling dup fd: " << syncFenceFd;
        struct pollfd fds;
        fds.fd = syncFenceFd;
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
        LOG(INFO) << "End of polling dup fd: " << syncFenceFd;

        if (fencedCallback != nullptr) {
            LOG(INFO) << "Try to get execution info: " << syncFenceFd;
            executionStatus = fencedCallback->getExecutionInfo();
            EXPECT_EQ(executionStatus, ErrorStatus::NONE);
        }
    } else {
        ErrorStatus executionStatus = g_service->execute(modelId, {input}, {output});
        EXPECT_EQ(executionStatus, ErrorStatus::NONE);
    }

    EXPECT_EQ(AHardwareBuffer_lock(
                  ahwbOutput,
                  AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1,
                  nullptr, &outputBuffer),
              0);
    if (outputBuffer != nullptr) {
        os.write(reinterpret_cast<char*>(outputBuffer), kOutputSize);
    }

    return true;
}

TEST_F(NpAgentTest, PreparedModelFromDlaPath) {
    int32_t modelId = prepareModelFromDlaPath();
    LOG(INFO) << "Release mode id: " << modelId;
    ErrorStatus status = g_service->releaseModel(modelId);
    EXPECT_EQ(status, ErrorStatus::NONE);
}

TEST_F(NpAgentTest, compute) {
    LOG(INFO) << "Load DLA";
    int32_t modelId = prepareModelFromDlaPath();
    LOG(INFO) << "Compute";
    compute(modelId, false);
    ErrorStatus status = g_service->releaseModel(modelId);
    EXPECT_EQ(status, ErrorStatus::NONE);
}

TEST_F(NpAgentTest, computeWithFence) {
    LOG(INFO) << "Load DLA";
    int32_t modelId = prepareModelFromDlaPath();
    LOG(INFO) << "Compute with fence";
    compute(modelId, true);
    ErrorStatus status = g_service->releaseModel(modelId);
    EXPECT_EQ(status, ErrorStatus::NONE);
}
