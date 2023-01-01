/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 *
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

#define LOG_TAG "NpAgent"

#include "NpAgent.h"

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android-base/properties.h>
#include <android-base/scopeguard.h>
#include <android/sync.h>
#include <ui/gralloc_extra.h>
#include <vndk/hardware_buffer.h>
#include <fstream>

#include "Executor.h"
#include "MemoryUtils.h"
#include "Model.h"
#include "ModelAttributes.h"
#include "ModelParams.h"
#include "NpAgentTypes.h"
#include "ServiceManager.h"
#include "SyncFence.h"
#include "Trace.h"

using mediatek::neuropilotagent::Executor;
using mediatek::neuropilotagent::Model;
using mediatek::neuropilotagent::ModelAttributes;
using mediatek::neuropilotagent::ModelParams;
using mediatek::neuropilotagent::SyncFence;

int NpAgent_getCompiledModelSize(int agentId, size_t* size) {
    NPAGENT_ATRACE_CALL();
    Model m;
    *size = m.GetCompilationSize(agentId);
    return RESULT_NO_ERROR;
}

int NpAgent_storeCompiledModel(int agentId, void* buffer, size_t size) {
    NPAGENT_ATRACE_CALL();
    Model m;
    m.StorePreparedModel(agentId, buffer, size);
    return RESULT_NO_ERROR;
}

int NpAgentOptions_create(NpAgentOptions** options) {
    NPAGENT_ATRACE_CALL();
    if (options == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }
    ModelParams* p = new ModelParams();
    *options = reinterpret_cast<NpAgentOptions*>(p);

    return RESULT_NO_ERROR;
}

void NpAgentOptions_release(NpAgentOptions* options) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    delete p;
}

int NpAgentOptions_setPoolSize(NpAgentOptions* options, uint32_t size) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_POOL_SIZE, size);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setBoostValue(NpAgentOptions* options, uint32_t value) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_BOOST_VALUE, value);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setInputFormat(NpAgentOptions* options, uint32_t format) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_INPUT_FORMAT, format);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setOutputFormat(NpAgentOptions* options, uint32_t format) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_OUTPUT_FORMAT, format);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setInputCompressionMode(NpAgentOptions* options, uint32_t mode) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_INPUT_COMPRESSION, mode);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setOutputCompressionMode(NpAgentOptions* options, uint32_t mode) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_OUTPUT_COMPRESSION, mode);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setInputHeightWidth(NpAgentOptions* options, uint32_t height, uint32_t width) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_INPUT_HEIGHT, height);
    p->SetParam(OP_INPUT_WIDTH, width);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setOutputHeightWidth(NpAgentOptions* options, uint32_t height, uint32_t width) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_OUTPUT_HEIGHT, height);
    p->SetParam(OP_OUTPUT_WIDTH, width);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setInputStride(NpAgentOptions* options, uint32_t stride) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_INPUT_W_STRIDE, stride);

    return RESULT_NO_ERROR;
}

int NpAgentOptions_setOutputStride(NpAgentOptions* options, uint32_t stride) {
    NPAGENT_ATRACE_CALL();
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    p->SetParam(OP_OUTPUT_W_STRIDE, stride);

    return RESULT_NO_ERROR;
}

int NpAgent_createFromBuffer(const void* buffer, size_t size, NpAgentOptions* options) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_createFromBuffer()";
    return 0;
}

int NpAgent_createFromFd(int32_t fd, size_t size, NpAgentOptions* options) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_createFromFd()";
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    Model m(fd, size, *p);

    return m.Build();
}

int NpAgent_createFromCacheBuffer(const void* buffer, size_t size, NpAgentOptions* options) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_createFromCacheBuffer()";
    return 0;
}

int NpAgent_createFromCacheFd(int32_t fd, size_t size, NpAgentOptions* options) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_createFromCacheFd()";
    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    Model m(fd, size, *p);

    return m.Build();
}

void NpAgent_release(int32_t agentId) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_release()";
    Model m;
    m.Release(agentId);
#if !defined(NDEBUG)
    std::ostringstream output;
    profiler::NpProfiler::GetInstance().Print(output, 20);
    LOG(INFO) << output.str();
#endif
}

int NpAgent_compute(int agentId, NpAgentExecution* execution) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_compute()";
    Executor* e = reinterpret_cast<Executor*>(execution);
    Model m;

    return (m.Inference(agentId, *e) ? RESULT_NO_ERROR : RESULT_OP_FAILED);
}

int NpAgent_computeWithFence(int agentId, NpAgentExecution* execution, int inputFence,
                             int outputFence, int* releaseFence, uint64_t duration) {
    NPAGENT_ATRACE_CALL();
    UNUSED(duration);
    LOG(INFO) << "NpAgent_computeWithFence(input/output): " << inputFence << "/" << outputFence;
    std::ostringstream ssInput;
    mediatek::neuropilotagent::getFdInfo(inputFence, &ssInput);
    LOG(DEBUG) << "inputFence info: " << ssInput.str();
    std::ostringstream ssOutput;
    mediatek::neuropilotagent::getFdInfo(outputFence, &ssOutput);
    LOG(DEBUG) << "outputFence info: " << ssOutput.str();
    Executor* e = reinterpret_cast<Executor*>(execution);
    Model m;

    auto waitInferenceDone =
        android::base::GetBoolProperty("debug.npagent.client.WaitInferenceDone", false);

    int waitFor = -1;
    if (inputFence > 0 && outputFence > 0) {
        waitFor = sync_merge("NpAgent", inputFence, outputFence);
        LOG(DEBUG) << "merge: " << inputFence << "/" << outputFence << " to " << waitFor;
        close(outputFence);
        LOG(DEBUG) << "close outputFence: " << outputFence;
    } else if (inputFence > 0 && outputFence < 0) {
        waitFor = dup(inputFence);
        LOG(DEBUG) << "dup: " << inputFence << " to " << waitFor;
    } else {
        waitFor = 0;
        LOG(WARNING) << "Unexpected condition, inputFence: " << inputFence
                     << " outputFence: " << outputFence;
    }

    LOG(DEBUG) << "wait fence: " << waitFor;
    e->SetWaitForFd(waitFor);

    std::vector<int> waitForVec;
    if (waitFor != -1) {
        waitForVec.push_back(waitFor);
    }

    bool ret = m.InferenceFenced(agentId, *e, waitForVec, releaseFence);
    LOG(DEBUG) << "get inference done fence: " << *releaseFence;
    if (ret) {
        std::ostringstream ssInferenceDone;
        mediatek::neuropilotagent::getFdInfo(*releaseFence, &ssInferenceDone);
        LOG(DEBUG) << "inference done fence info: " << ssInferenceDone.str();
    }

    if (waitInferenceDone) {
        auto fence = SyncFence::Create(android::base::unique_fd(*releaseFence));
        auto r = fence.SyncWait({/* no timeout */});
        if (r != SyncFence::FenceState::SIGNALED) {
            LOG(ERROR) << "syncWait failed, fd: " << fence.GetFd()
                       << ", state: " << static_cast<uint8_t>(r);
        }
        *releaseFence = -1;
        LOG(DEBUG) << "return inference done fence: " << *releaseFence << " to caller";
    }
    return (ret ? RESULT_NO_ERROR : RESULT_OP_FAILED);
}

int NpAgentExecution_create(NpAgentExecution** execution) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentExecution_create()";
    Executor* e = new Executor();
    if (e == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    *execution = reinterpret_cast<NpAgentExecution*>(e);
    return RESULT_NO_ERROR;
}

void NpAgentExecution_release(NpAgentExecution* execution) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentExecution_release()";
    Executor* e = reinterpret_cast<Executor*>(execution);
    delete e;
}

int NpAgentExecution_setInput(NpAgentExecution* execution, int inputFd, size_t size) {
    NPAGENT_ATRACE_CALL();
    if (inputFd < 0) {
        LOG(ERROR) << "NpAgentExecution_setInput(), unexpected fd: " << inputFd;
        return RESULT_BAD_DATA;
    }
    LOG(DEBUG) << "NpAgentExecution_setInput(), fd: " << inputFd;
    Executor* e = reinterpret_cast<Executor*>(execution);
    e->SetInputs({inputFd}, {static_cast<uint32_t>(size)});
    return RESULT_NO_ERROR;
}

int NpAgentExecution_setOutput(NpAgentExecution* execution, int outputFd, size_t size) {
    NPAGENT_ATRACE_CALL();
    if (outputFd < 0) {
        LOG(ERROR) << "NpAgentExecution_setOutput(), unexpected fd: " << outputFd;
        return RESULT_BAD_DATA;
    }
    LOG(DEBUG) << "NpAgentExecution_setOutput(), fd: " << outputFd;
    Executor* e = reinterpret_cast<Executor*>(execution);
    e->SetOutputs({outputFd}, {static_cast<uint32_t>(size)});
    return RESULT_NO_ERROR;
}

int NpAgentAttributes_create(int agentId, NpAgentAttributes** attributes) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_create()";
    if (attributes == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = new ModelAttributes(agentId);
    if (attr == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    *attributes = reinterpret_cast<NpAgentAttributes*>(attr);
    return RESULT_NO_ERROR;
}

void NpAgentAttributes_release(NpAgentAttributes* attributes) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_release()";
    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    delete attr;
}

int NpAgentAttributes_getInputFormat(NpAgentAttributes* attributes, uint32_t* format) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_getInputFormat()";
    if (format == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    if (!attr->GetInputFormat(format)) {
        return RESULT_BAD_DATA;
    }

    return RESULT_NO_ERROR;
}

int NpAgentAttributes_getOutputFormat(NpAgentAttributes* attributes, uint32_t* format) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_getOutputFormat()";
    if (format == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    if (!attr->GetOutputFormat(format)) {
        return RESULT_BAD_DATA;
    }

    return RESULT_NO_ERROR;
}

int NpAgentAttributes_getInputCompressionMode(NpAgentAttributes* attributes, uint32_t* mode) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_getInputCompressionMode()";
    if (attributes == nullptr || mode == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    if (!attr->GetInputCompressionMode(mode)) {
        return RESULT_BAD_DATA;
    }

    return RESULT_NO_ERROR;
}

int NpAgentAttributes_getOutputCompressionMode(NpAgentAttributes* attributes, uint32_t* mode) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_getOutputCompressionMode()";
    if (attributes == nullptr || mode == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    if (!attr->GetOutputCompressionMode(mode)) {
        return RESULT_BAD_DATA;
    }

    return RESULT_NO_ERROR;
}

int NpAgentAttributes_getInputHeightWidth(NpAgentAttributes* attributes, uint32_t* height,
                                          uint32_t* width) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_getInputHeightWidth()";
    if (attributes == nullptr || height == nullptr || width == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    if (!attr->GetInputHeight(height) || !attr->GetInputWidth(width)) {
        return RESULT_BAD_DATA;
    }
    return RESULT_NO_ERROR;
}

int NpAgentAttributes_getOutputHeightWidth(NpAgentAttributes* attributes, uint32_t* height,
                                           uint32_t* width) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_getInputHeightWidth()";
    if (attributes == nullptr || height == nullptr || width == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    if (!attr->GetOutputHeight(height) || !attr->GetOutputWidth(width)) {
        return RESULT_BAD_DATA;
    }
    return RESULT_NO_ERROR;
}

int NpAgentAttributes_getInputStride(NpAgentAttributes* attributes, uint32_t* stride) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_getInputStride()";
    if (attributes == nullptr || stride == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    if (!attr->GetInputStride(stride)) {
        return RESULT_BAD_DATA;
    }

    return RESULT_NO_ERROR;
}

int NpAgentAttributes_getOutputStride(NpAgentAttributes* attributes, uint32_t* stride) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgentAttributes_getOutputStride()";
    if (attributes == nullptr || stride == nullptr) {
        return RESULT_UNEXPECTED_NULL;
    }

    ModelAttributes* attr = reinterpret_cast<ModelAttributes*>(attributes);
    if (!attr->GetOutputStride(stride)) {
        return RESULT_BAD_DATA;
    }

    return RESULT_NO_ERROR;
}

int NpAgent_validateInput(int agentId, uint32_t format, uint32_t height, uint32_t width,
                          uint32_t stride) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_validateInput()";
    ModelAttributes attr(agentId);

    uint32_t attrHeight = 0;
    uint32_t attrWidth = 0;
    uint32_t attrStride = 0;

    if (!attr.GetInputHeight(&attrHeight) || !attr.GetInputWidth(&attrWidth) ||
        !attr.GetInputStride(&attrStride) || attrHeight != height || attrWidth != width ||
        attrStride != stride) {
        return RESULT_BAD_DATA;
    }
    return RESULT_NO_ERROR;
}

int NpAgent_validateOutput(int agentId, uint32_t format, uint32_t height, uint32_t width,
                           uint32_t stride) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_validateOutput()";
    ModelAttributes attr(agentId);

    uint32_t attrHeight = 0;
    uint32_t attrWidth = 0;
    uint32_t attrStride = 0;

    if (!attr.GetOutputHeight(&attrHeight) || !attr.GetOutputWidth(&attrWidth) ||
        !attr.GetOutputStride(&attrStride) || attrHeight != height || attrWidth != width ||
        attrStride != stride) {
        return RESULT_BAD_DATA;
    }
    return RESULT_NO_ERROR;
}

int NpAgent_validateInputHeightWidth(int agentId, uint32_t height, uint32_t width) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_validateInputHeightWidth()";
    ModelAttributes attr(agentId);

    uint32_t attrHeight = 0;
    uint32_t attrWidth = 0;

    if (!attr.GetInputHeight(&attrHeight) || !attr.GetInputWidth(&attrWidth) ||
        attrHeight != height || attrWidth != width) {
        return RESULT_BAD_DATA;
    }
    return RESULT_NO_ERROR;
}

int NpAgent_validateOutputHeightWidth(int agentId, uint32_t height, uint32_t width) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "NpAgent_checNpAgent_validateOutputkOutput()";
    ModelAttributes attr(agentId);

    uint32_t attrHeight = 0;
    uint32_t attrWidth = 0;

    if (!attr.GetOutputHeight(&attrHeight) || !attr.GetOutputWidth(&attrWidth) ||
        attrHeight != height || attrWidth != width) {
        return RESULT_BAD_DATA;
    }
    return RESULT_NO_ERROR;
}

int NpAgent_updateOptions(int agentId, NpAgentOptions* options, uint32_t optionCode) {
    UNUSED(optionCode);

    ModelParams* p = reinterpret_cast<ModelParams*>(options);
    Model m;
    m.Update(agentId, *p);
    return RESULT_NO_ERROR;
}

int NpAgent_gpuCreate(const buffer_handle_t& handle) {
    int err = GRALLOC_EXTRA_OK;
    ge_nn_model_info_t nn_model_info;
    err |= gralloc_extra_query(handle, GRALLOC_EXTRA_GET_NN_MODEL_INFO, &nn_model_info);
    if (err != GRALLOC_EXTRA_OK) {
        return -1;
    }

    // Read DLA
    std::string dlaPath(nn_model_info.dlapath);
    std::ifstream is(dlaPath, std::ios::binary | std::ios::ate);
    if (!is) {
        LOG(ERROR) << "Fail to read DLA file";
        return -1;
    }

    const size_t size = (size_t)is.tellg();
    if (0 >= size) {
        LOG(ERROR) << "Invalid DLA size: " << size;
        return -1;
    }
    LOG(DEBUG) << "DLA size: " << size;

    AHardwareBuffer* ahwb = nullptr;
    AHardwareBuffer_Desc desc{
        .width = static_cast<uint32_t>(size),
        .height = 1,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_BLOB,
        .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
    };

    int fd = -1;
    void* buffer = nullptr;

    auto ahwbGuard = android::base::make_scope_guard([ahwb]() {
        if (ahwb != nullptr) {
            AHardwareBuffer_unlock(ahwb, nullptr);
            AHardwareBuffer_release(ahwb);
        }
    });

    if (AHardwareBuffer_allocate(&desc, &ahwb) != 0) {
        return -1;
    }

    if (AHardwareBuffer_lock(
            ahwb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1,
            nullptr, &buffer) != 0) {
        return -1;
    }

    is.seekg(0, is.beg);
    is.read(reinterpret_cast<char*>(buffer), (std::streamsize)size);

    AHardwareBuffer_unlock(ahwb, nullptr);

    if (gralloc_extra_query(AHardwareBuffer_getNativeHandle(ahwb), GRALLOC_EXTRA_GET_ION_FD, &fd) !=
        GRALLOC_EXTRA_OK) {
        return -1;
    }

    // Start to create agent
    NpAgentOptions* options = nullptr;
    NpAgentOptions_create(&options);

    auto npAgentOptionsGuard = android::base::make_scope_guard([options]() {
        if (options != nullptr) {
            NpAgentOptions_release(options);
        }
    });

    if (NpAgentOptions_setPoolSize(options, nn_model_info.pool_size) != RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setBoostValue(options, nn_model_info.boost) != RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setInputHeightWidth(options, nn_model_info.ipt_h, nn_model_info.ipt_w) !=
        RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setInputStride(options, nn_model_info.ipt_strd) != RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setInputFormat(options, nn_model_info.ipt_fmt) != RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setInputCompressionMode(options, nn_model_info.ipt_cprs) !=
        RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setOutputHeightWidth(options, nn_model_info.opt_h, nn_model_info.opt_w) !=
        RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setOutputStride(options, nn_model_info.opt_strd) != RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setOutputFormat(options, nn_model_info.opt_fmt) != RESULT_NO_ERROR) {
        return -1;
    }

    if (NpAgentOptions_setOutputCompressionMode(options, nn_model_info.opt_cprs) !=
        RESULT_NO_ERROR) {
        return -1;
    }

    return NpAgent_createFromCacheFd(fd, size, options);
}

int NpAgent_gpuUpdate(int agentId, const buffer_handle_t& handle, uint32_t optionCode) {
    UNUSED(optionCode);

    int err = GRALLOC_EXTRA_OK;
    ge_nn_model_info_t nn_model_info;
    err |= gralloc_extra_query(handle, GRALLOC_EXTRA_GET_NN_MODEL_INFO, &nn_model_info);
    if (err != GRALLOC_EXTRA_OK) {
        LOG(ERROR) << "Fail to get boost value from buffer handle";
        return -1;
    }

    // TODO: Support changing boost value currently
    NpAgentOptions* options = nullptr;
    NpAgentOptions_create(&options);

    auto npAgentOptionsGuard = android::base::make_scope_guard([options]() {
        if (options != nullptr) {
            NpAgentOptions_release(options);
        }
    });

    if (NpAgentOptions_setBoostValue(options, nn_model_info.boost) != RESULT_NO_ERROR) {
        LOG(DEBUG) << "GPU tries to update boost value: " << nn_model_info.boost;
        return -1;
    }

    return NpAgent_updateOptions(agentId, options, OPTION_BOOST_VALUE);
}
