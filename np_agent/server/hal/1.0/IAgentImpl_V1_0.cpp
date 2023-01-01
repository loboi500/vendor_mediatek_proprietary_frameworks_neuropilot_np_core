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

#define LOG_TAG "IAgentImpl_V1_0"

#include "IAgentImpl_V1_0.h"

#include <android-base/logging.h>
#include <sys/mman.h>
#include <ui/gralloc_extra.h>

#include <vector>

#include "CpuUtils.h"
#include "ModelFactory.h"
#include "SharedMemory.h"
#include "Trace.h"
#include "Utils.h"

namespace mediatek {
namespace neuropilotagent {

Return<int32_t> IAgentImpl_V1_0::prepareModel(const hidl_memory& hidlMemory,
                                              const Options& options) {
    NPAGENT_ATRACE_CALL();
    const auto& memType = hidlMemory.name();
    if (memType != "ion_fd") {
        return -1;
    }
    size_t size = hidlMemory.size();
    int fd = hidlMemory.handle()->data[0];
    LOG(DEBUG) << "prepareModel, model fd: " << fd << " size: " << size;
    return ModelFactory::GetInstance().PrepareModel(fd, size, options.outputStride);
}

Return<int32_t> IAgentImpl_V1_0::prepareModelFromCache(const hidl_memory& hidlMemory,
                                                       const Options& options) {
    NPAGENT_ATRACE_CALL();
    const auto& memType = hidlMemory.name();
    auto handle = hidlMemory.handle();
    size_t size = hidlMemory.size();
    int fd = -1;
    if (memType == "ion_fd") {
        fd = handle->data[0];
    } else if (memType == "hardware_buffer_blob") {
        int err = ::gralloc_extra_query(handle, GRALLOC_EXTRA_GET_ION_FD, &fd);
        if (err != GRALLOC_EXTRA_OK || fd <= 0) {
            return -1;
        }
    } else {
        LOG(ERROR) << "Unknown memType: " << memType << " to prepare model from cache";
        return -1;
    }
    LOG(DEBUG) << "prepareModelFromCache, cache fd: " << fd << " size: " << size;
    LOG(DEBUG) << "compressionMode: " << options.compressionMode;
    LOG(DEBUG) << "bufferFormat: " << options.bufferFormat;
    LOG(DEBUG) << "inputStride: " << options.inputStride;
    LOG(DEBUG) << "outputStride: " << options.outputStride;

    return ModelFactory::GetInstance().PrepareModelFromCache(fd, size, options);
}

Return<uint32_t> IAgentImpl_V1_0::getPreparedModelSize(int32_t modelId) {
    NPAGENT_ATRACE_CALL();
    return ModelFactory::GetInstance().GetPreparedModelSize(modelId);
}

Return<ErrorStatus> IAgentImpl_V1_0::storePreparedModel(int32_t modelId,
                                                        const hidl_memory& hidlMemory) {
    NPAGENT_ATRACE_CALL();
    const auto& memType = hidlMemory.name();
    auto handle = hidlMemory.handle();
    size_t size = hidlMemory.size();
    int fd = -1;
    if (memType == "ion_fd") {
        fd = hidlMemory.handle()->data[0];
    } else if (memType == "hardware_buffer_blob") {
        int err = ::gralloc_extra_query(handle, GRALLOC_EXTRA_GET_ION_FD, &fd);
        if (err != GRALLOC_EXTRA_OK || fd <= 0) {
            return ErrorStatus::INVALID_ARGUMENT;
        }
    }
    LOG(DEBUG) << "storePreparedModel, buffer fd: " << fd << " size: " << size;
    bool success = ModelFactory::GetInstance().StorePreparedModel(modelId, fd, size);
    return success ? ErrorStatus::NONE : ErrorStatus::INVALID_ARGUMENT;
}

Return<ErrorStatus> IAgentImpl_V1_0::releaseModel(int32_t modelId) {
    NPAGENT_ATRACE_CALL();
#if !defined(NDEBUG)
    std::ostringstream output;
    profiler::NpProfiler::GetInstance().Print(output, 20);
    LOG(INFO) << output.str();
#endif
    return ModelFactory::GetInstance().ReleaseModel(modelId) ? ErrorStatus::NONE
                                                             : ErrorStatus::INVALID_ARGUMENT;
}

Return<ErrorStatus> IAgentImpl_V1_0::execute(int32_t modelId, const hidl_vec<hidl_memory>& inputs,
                                             const hidl_vec<hidl_memory>& outputs) {
    NPAGENT_ATRACE_CALL();
    std::vector<SharedMemory> inputsMemory;
    std::vector<SharedMemory> outputsMemory;

    // Process inputs
    for (size_t i = 0; i < inputs.size(); i++) {
        const auto& memType = inputs[i].name();
        if (memType != "ion_fd") {
            return ErrorStatus::INVALID_ARGUMENT;
        }
        size_t size = inputs[i].size();
        int fd = inputs[i].handle()->data[0];
        auto memory = createSharedMemoryFromFd(size, PROT_READ, fd, 0);
        inputsMemory.push_back(memory.value());
    }

    // Process outputs
    for (size_t i = 0; i < outputs.size(); i++) {
        const auto& memType = outputs[i].name();
        if (memType != "ion_fd") {
            return ErrorStatus::INVALID_ARGUMENT;
        }
        size_t size = outputs[i].size();
        int fd = outputs[i].handle()->data[0];
        auto memory = createSharedMemoryFromFd(size, PROT_WRITE, fd, 0);
        outputsMemory.push_back(memory.value());
    }

    return ModelFactory::GetInstance().Execute(modelId, inputsMemory, outputsMemory)
               ? ErrorStatus::NONE
               : ErrorStatus::INVALID_ARGUMENT;
}

Return<ErrorStatus> IAgentImpl_V1_0::executeFenced(int32_t modelId,
                                                   const hidl_vec<hidl_memory>& inputs,
                                                   const hidl_vec<hidl_memory>& outputs,
                                                   const hidl_vec<hidl_handle>& waitFor,
                                                   int* outputFence) {
    NPAGENT_ATRACE_CALL();
    if (mBindCpuLittleCores) {
        BindCpuLittleCores();
    }

    std::vector<SharedMemory> inputsMemory;
    std::vector<SharedMemory> outputsMemory;
    std::vector<int> inputsFence;

    // Process inputs
    for (size_t i = 0; i < inputs.size(); i++) {
        const auto& memType = inputs[i].name();
        if (memType != "ion_fd") {
            return ErrorStatus::INVALID_ARGUMENT;
        }
        size_t size = inputs[i].size();
        int fd = inputs[i].handle()->data[0];
        auto memory = createSharedMemoryFromFd(size, PROT_READ, fd, 0);
        inputsMemory.push_back(memory.value());
    }

    // Process outputs
    for (size_t i = 0; i < outputs.size(); i++) {
        const auto& memType = outputs[i].name();
        if (memType != "ion_fd") {
            return ErrorStatus::INVALID_ARGUMENT;
        }
        size_t size = outputs[i].size();
        int fd = outputs[i].handle()->data[0];
        auto memory = createSharedMemoryFromFd(size, PROT_WRITE, fd, 0);
        outputsMemory.push_back(memory.value());
    }

    // Process input fences
    for (size_t i = 0; i < waitFor.size(); i++) {
        inputsFence.push_back(waitFor[i]->data[0]);
    }

    return ModelFactory::GetInstance().ExecuteFenced(modelId, inputsMemory, outputsMemory,
                                                     inputsFence, outputFence)
               ? ErrorStatus::NONE
               : ErrorStatus::INVALID_ARGUMENT;
}

std::optional<Attributes> IAgentImpl_V1_0::getAttributes(int32_t modelId) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "getAttributes, modelId: " << modelId;
    return ModelFactory::GetInstance().GetAttributes(modelId);
}

Return<ErrorStatus> IAgentFencedExecutionCallbackImpl_V1_0::getExecutionInfo() {
    LOG(DEBUG) << "getExecutionInfo, modelId: " << mModelId;

    return ErrorStatus::NONE;
}

}  // namespace neuropilotagent
}  // namespace mediatek
