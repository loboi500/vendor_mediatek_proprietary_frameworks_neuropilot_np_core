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

#define LOG_TAG "NpAgentServiceManager"

#include "ServiceManager.h"

#include <android-base/logging.h>

#include "HalInterfaces.h"
#include "MemoryUtils.h"

namespace mediatek {
namespace neuropilotagent {

ServiceManager* ServiceManager::GetInstance() {
    static ServiceManager manager;
    return &manager;
}

ServiceManager::ServiceManager() {
    mService = V_IAgent::getService();
    if (mService == nullptr) {
        LOG(ERROR) << "fail to get IAgentService";
    }
}

int32_t ServiceManager::PrepareModel(int32_t fd, uint32_t size, const V_Options& options) {
    hidl_memory hidlMemory = createHidlMemoryFromFd(fd, size);

    int32_t modelId = mService->prepareModel(hidlMemory, options);
    releaseHidlMemoryFromFd(hidlMemory);

    return modelId;
}

int32_t ServiceManager::PrepareModelFromCache(int32_t fd, uint32_t size, const V_Options& options) {
    hidl_memory hidlMemory = createHidlMemoryFromFd(fd, size);

    int32_t modelId = mService->prepareModelFromCache(hidlMemory, options);
    releaseHidlMemoryFromFd(hidlMemory);

    return modelId;
}

int32_t ServiceManager::PrepareModelFromCache(int32_t fd, uint32_t size,
                                              const std::vector<uint32_t>& params) {
    hidl_memory hidlMemory = createHidlMemoryFromFd(fd, size);

    int32_t modelId = mService->prepareModelFromCache_1_1(hidlMemory, params);
    releaseHidlMemoryFromFd(hidlMemory);

    return modelId;
}

bool ServiceManager::Execute(int32_t modelId, const std::vector<int32_t>& inputs,
                             const std::vector<uint32_t>& inputSizes,
                             const std::vector<int32_t>& outputs,
                             const std::vector<uint32_t>& outputSizes) {
    if (inputs.size() != inputSizes.size()) {
        LOG(ERROR) << "invalid inputs";
        return false;
    }

    if (outputs.size() != outputSizes.size()) {
        LOG(ERROR) << "invalid outputs";
        return false;
    }

    hidl_vec<hidl_memory> inputMemories(inputSizes.size());
    hidl_vec<hidl_memory> outputMemories(outputSizes.size());

    for (size_t i = 0; i < inputs.size(); i++) {
        inputMemories[i] = createHidlMemoryFromFd(inputs[i], inputSizes[i]);
    }

    for (size_t i = 0; i < outputs.size(); i++) {
        outputMemories[i] = createHidlMemoryFromFd(outputs[i], outputSizes[i]);
    }

    V_ErrorStatus status = mService->execute(modelId, inputMemories, outputMemories);

    for (size_t i = 0; i < inputs.size(); i++) {
        releaseHidlMemoryFromFd(inputMemories[i]);
    }

    for (size_t i = 0; i < outputs.size(); i++) {
        releaseHidlMemoryFromFd(outputMemories[i]);
    }
    return (status == V_ErrorStatus::NONE ? true : false);
}

bool ServiceManager::ExecuteFenced(int32_t modelId, const std::vector<int32_t>& inputs,
                                   const std::vector<uint32_t>& inputSizes,
                                   const std::vector<int32_t>& outputs,
                                   const std::vector<uint32_t>& outputSizes,
                                   std::vector<int>& inputFence, int* outputFence) {
    if (inputs.size() != inputSizes.size()) {
        LOG(ERROR) << "invalid inputs";
        return false;
    }

    if (outputs.size() != outputSizes.size()) {
        LOG(ERROR) << "invalid outputs";
        return false;
    }

    if (outputFence == nullptr) {
        LOG(ERROR) << "invalid output fence";
        return false;
    }

    hidl_vec<hidl_memory> inputMemories(inputSizes.size());
    hidl_vec<hidl_memory> outputMemories(outputSizes.size());

    for (size_t i = 0; i < inputs.size(); i++) {
        inputMemories[i] = createHidlMemoryFromFd(inputs[i], inputSizes[i]);
    }

    for (size_t i = 0; i < outputs.size(); i++) {
        outputMemories[i] = createHidlMemoryFromFd(outputs[i], outputSizes[i]);
    }

    V_ErrorStatus result = V_ErrorStatus::NONE;
    hidl_handle syncFenceHandle;
    sp<V_IFencedExecutionCallback> fencedCallback = nullptr;
    auto callbackFunc = [&result, &syncFenceHandle, &fencedCallback](
                            V_ErrorStatus error, const hidl_handle& handle,
                            const sp<V_IFencedExecutionCallback>& callback) {
        result = error;
        syncFenceHandle = handle;
        fencedCallback = callback;
        LOG(INFO) << "cb, result: " << static_cast<uint32_t>(result);
    };

    std::vector<hidl_handle> waitFor;
    if (inputFence.size() > 0) {
        // TODO: Suport multiple input fence fds
        native_handle_t* nativeHandle = native_handle_create(1, 0);
        if (nativeHandle == nullptr) {
            LOG(ERROR) << "Fail to create native handle";
            return false;
        }
        nativeHandle->data[0] = dup(inputFence[0]);
        LOG(DEBUG) << "dup wait fence : " << inputFence[0] << " to " << nativeHandle->data[0];
        hidl_handle hidlHandle;
        hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
        waitFor.push_back(hidlHandle);
    }

    Return<void> ret =
        mService->executeFenced(modelId, inputMemories, outputMemories, waitFor, callbackFunc);
    if (!ret.isOk() || result != V_ErrorStatus::NONE) {
        return false;
    }

    if (syncFenceHandle.getNativeHandle() != nullptr &&
        syncFenceHandle.getNativeHandle()->data[0] > 0) {
        *outputFence = dup(syncFenceHandle.getNativeHandle()->data[0]);
        LOG(INFO) << "Dup inference done fence: " << syncFenceHandle.getNativeHandle()->data[0]
                  << " to " << *outputFence;
    } else {
        *outputFence = 0;
        LOG(INFO) << "Get inference done fence: " << *outputFence;
    }

    for (size_t i = 0; i < inputs.size(); i++) {
        releaseHidlMemoryFromFd(inputMemories[i]);
    }

    for (size_t i = 0; i < outputs.size(); i++) {
        releaseHidlMemoryFromFd(outputMemories[i]);
    }
    return (*outputFence > 0);
}

void ServiceManager::ReleaseModel(int32_t modelId) { mService->releaseModel(modelId); }

uint32_t ServiceManager::GetPreparedModelSize(int32_t modelId) {
    return mService->getPreparedModelSize(modelId);
}

bool ServiceManager::StorePreparedModel(int32_t modelId, void* buffer, size_t size) {
    void* cacheBuffer = nullptr;
    AHardwareBuffer* ahwb = nullptr;
    AHardwareBuffer_Desc desc{
        .width = static_cast<uint32_t>(size),
        .height = 1,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_BLOB,
        .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
        .stride = static_cast<uint32_t>(size),
    };

    if (AHardwareBuffer_allocate(&desc, &ahwb) != android::NO_ERROR) {
        return false;
    }

    hidl_memory hidlMemory = createHidlMemoryFromAHardwareBuffer(ahwb);

    V_ErrorStatus status = mService->storePreparedModel(modelId, hidlMemory);

    if (status == V_ErrorStatus::NONE &&
        AHardwareBuffer_lock(
            ahwb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1,
            nullptr, &cacheBuffer) == android::NO_ERROR) {
        memcpy(buffer, cacheBuffer, size);
        AHardwareBuffer_unlock(ahwb, nullptr);
    }

    releaseHidlMemoryFromAHardwareBuffer(hidlMemory);
    AHardwareBuffer_release(ahwb);
    return status == V_ErrorStatus::NONE ? true : false;
}

std::optional<V_Attributes> ServiceManager::GetAttributes(int32_t modelId) {
    bool valid = false;
    V_Attributes attr;
    Return<void> ret = mService->getAttributes(
        modelId, [&valid, &attr](V_ErrorStatus status, const V_Attributes& attributes) {
            if (status == V_ErrorStatus::NONE) {
                valid = true;
                attr = attributes;
            }
        });
    if (valid && ret.isOk()) {
        return attr;
    }
    return std::nullopt;
}

std::vector<uint32_t> ServiceManager::GetParams(int32_t modelId) {
    // Use cached params if available
    auto cachedParams = mParamsMap.find(modelId);
    if (cachedParams != mParamsMap.end()) {
        LOG(DEBUG) << "Get cached params for model id: " << modelId;
        return cachedParams->second;
    }

    std::vector<uint32_t> params;
    Return<void> ret = mService->getParams(
        modelId, [&params](V_ErrorStatus status, const std::vector<uint32_t>& modelParams) {
            if (status == V_ErrorStatus::NONE) {
                params = modelParams;
            }
        });
    if (!ret.isOk()) {
        return {};
    }
    // Cache the params
    mParamsMap[modelId] = params;
    return params;
}

bool ServiceManager::SetParams(int32_t modelId, const std::vector<uint32_t>& params) {
    bool update = false;

    auto currentParams = GetParams(modelId);
    auto newParams = currentParams;

    if (currentParams.size() == 0) {
        return false;
    }

    // Update current params if necessary
    for (size_t i = 0; i < params.size(); i++) {
        if (i % 2 != 0) {
            continue;
        }
        auto it = std::find(currentParams.begin(), currentParams.end(), params[i]);
        if (it == currentParams.end()) {
            // Insert new params
            newParams.push_back(params[i]);
            newParams.push_back(params[i + 1]);
            update = true;
            continue;
        }
        // Update existing params
        if (newParams[it - currentParams.begin() + 1] != params[i + 1]) {
            newParams[it - currentParams.begin() + 1] = params[i + 1];
            update = true;
        }
    }

    if (update) {
        LOG(DEBUG) << "SetParams for model id: " << modelId;
        for (size_t i = 0; i < newParams.size(); i++) {
            LOG(DEBUG) << newParams[i];
        }
        V_ErrorStatus status = mService->setParams(modelId, newParams);
        if (V_ErrorStatus::NONE == status) {
            // Cache the params
            mParamsMap[modelId] = newParams;
        }
        return (V_ErrorStatus::NONE == status);
    }

    LOG(DEBUG) << "No need to set params for model id: " << modelId;
    return true;
}

}  // namespace neuropilotagent
}  // namespace mediatek
