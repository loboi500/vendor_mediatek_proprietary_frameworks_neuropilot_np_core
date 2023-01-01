/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "ModelFactory"

#include "ModelFactory.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <sys/mman.h>

#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "NpAgentTypes.h"
#include "RuntimeOptions.h"
#include "SharedMemory.h"
#include "SyncFence.h"
#include "Trace.h"
#include "Utils.h"

namespace mediatek {
namespace neuropilotagent {

ModelInfo::~ModelInfo() {
    if (mRuntime) {
        LOG(DEBUG) << "Release Neuron runtime: " << std::hex << mRuntime;
        NeuronRuntime_release(mRuntime);
    }

    ClearFenceQueue();
}

void ModelInfo::EnqueueFence(int fd) {
    auto fence = SyncFence::Create(android::base::unique_fd(dup(fd)));
    LOG(DEBUG) << "EnqueueFence, fd: " << fence.GetFd();
    mFenceQueue.push(fence);
}

void ModelInfo::ClearFenceQueue() {
    while (!mFenceQueue.empty()) {
        auto fence = mFenceQueue.front();
        auto r = fence.SyncWait({/* no timeout */});
        if (r == SyncFence::FenceState::SIGNALED) {
            LOG(DEBUG) << "ClearFenceQueue, fd: " << fence.GetFd();
            mFenceQueue.pop();
        }
    }
}

RuntimeObject::~RuntimeObject() {
    if (mRuntime) {
        LOG(DEBUG) << "Release Neuron runtime: " << std::hex << mRuntime;
        NeuronRuntime_release(mRuntime);
    }
}

ModelFactory::ModelFactory() {
    mDumpInput = RuntimeOptions().DumpInput();
    mDumpOutput = RuntimeOptions().DumpOutput();

    LOG(DEBUG) << "mDumpInput: " << mDumpInput << " mDumpOutput:" << mDumpOutput;
}

ModelFactory::~ModelFactory() {}

int32_t ModelFactory::PrepareModel(int32_t fd, uint32_t size, uint32_t stride) {
    UNUSED(fd);
    UNUSED(size);
    UNUSED(stride);

    LOG(DEBUG) << "PrepareModel";

    return 1;
}

int32_t ModelFactory::PrepareModelFromCache(int32_t fd, uint32_t size, const Options& options) {
    UNUSED(fd);
    UNUSED(size);
    UNUSED(options);

    return -1;
}

int32_t ModelFactory::PrepareModelFromCache(int32_t fd, uint32_t size,
                                            const std::vector<uint32_t>& params) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "PrepareModelFromCache";

    auto memory = createSharedMemoryFromFd(size, PROT_READ, fd, 0);
    auto mapping = map(memory.value());
    if (!mapping.has_value()) {
        LOG(ERROR) << "Fail to map model cache buffer from fd: " << fd;
        return -1;
    }

    if (!std::holds_alternative<const void*>(mapping.value().pointer)) {
        LOG(ERROR) << "Shoule be constant buffer";
        return -1;
    }

    const void* buffer = std::get<const void*>(mapping.value().pointer);
    if (buffer == nullptr) {
        LOG(ERROR) << "Fail to get the mapped buffer from fd: " << fd;
        return -1;
    }

    void* runtime = nullptr;
    const auto numMdla = RuntimeOptions().NumMdla();

    // Setup the environment options for the Neuron Runtime
    EnvOptions envOptions = {
        .deviceKind = kEnvOptHardware,
        .MDLACoreOption = MDLACoreMode::Auto,
        .suppressInputConversion = false,
        .suppressOutputConversion = false,
    };

    if (NeuronRuntime_create(&envOptions, &runtime) != NEURONRUNTIME_NO_ERROR) {
        LOG(ERROR) << "Fail to create Neuron runtime";
        return -1;
    }

    if (NeuronRuntime_loadNetworkFromBuffer(runtime, buffer, size) != NEURONRUNTIME_NO_ERROR) {
        LOG(ERROR) << "Fail to load Neuron network from buffer";
        return -1;
    }

    size_t inputSize = 0;
    size_t outputSize = 0;
    if (NeuronRuntime_getSingleInputSize(runtime, &inputSize) == NEURONRUNTIME_NO_ERROR) {
        LOG(INFO) << "Input size: " << inputSize;
    }
    if (NeuronRuntime_getSingleOutputSize(runtime, &outputSize) == NEURONRUNTIME_NO_ERROR) {
        LOG(INFO) << "Output size: " << outputSize;
    }

    uint32_t param = 0;
    uint8_t boostValue = NEURONRUNTIME_BOOSTVALUE_MAX;
    if (GetParam(params, OP_INPUT_FORMAT, &param)) {
        LOG(DEBUG) << "Input format: " << param;
    }
    if (GetParam(params, OP_OUTPUT_FORMAT, &param)) {
        LOG(DEBUG) << "Output format: " << param;
    }
    if (GetParam(params, OP_INPUT_COMPRESSION, &param)) {
        LOG(DEBUG) << "Input compression mode: " << param;
    }
    if (GetParam(params, OP_OUTPUT_COMPRESSION, &param)) {
        LOG(DEBUG) << "Output compression mode: " << param;
    }
    if (GetParam(params, OP_INPUT_HEIGHT, &param)) {
        LOG(DEBUG) << "Input height: " << param;
    }
    if (GetParam(params, OP_INPUT_WIDTH, &param)) {
        LOG(DEBUG) << "Input width: " << param;
    }
    if (GetParam(params, OP_OUTPUT_HEIGHT, &param)) {
        LOG(DEBUG) << "Output height: " << param;
    }
    if (GetParam(params, OP_OUTPUT_WIDTH, &param)) {
        LOG(DEBUG) << "Output width: " << param;
    }
    if (GetParam(params, OP_INPUT_W_STRIDE, &param)) {
        LOG(DEBUG) << "Input stride: " << param;
    }
    if (GetParam(params, OP_OUTPUT_W_STRIDE, &param)) {
        LOG(DEBUG) << "Output stride: " << param;
    }
    if (GetParam(params, OP_BOOST_VALUE, &param)) {
        boostValue = static_cast<uint8_t>(param);
        LOG(DEBUG) << "Boost value: " << param;
    }

    // Lock mutex before insertion
    std::lock_guard<std::mutex> lock(mMutex);
    auto modelInfo = std::make_shared<ModelInfo>(runtime, size, params);
    modelInfo->SetBoostValue(boostValue);
    mModelInfos.insert({++mModelNumber, modelInfo});

    auto pool = std::make_shared<RuntimeObjectPool>();
    if (!GetParam(params, OP_POOL_SIZE, &param) || param == 0) {
        param = kDefaultPoolSize;
    }
    const auto poolSize = RuntimeOptions().PoolSize();
    if (poolSize > 0) {
        LOG(DEBUG) << "Use pool size: " << poolSize << " from debug options";
        param = poolSize;
    }

    LOG(DEBUG) << "Prepare model object pool(" << param << ") with model id: " << mModelNumber;
    for (auto i = 0; i < param; i++) {
        void* runtimeptr = nullptr;
        if (NeuronRuntime_create(&envOptions, &runtimeptr) != NEURONRUNTIME_NO_ERROR) {
            LOG(ERROR) << "Fail to create Neuron runtime";
            return -1;
        }
        if (NeuronRuntime_loadNetworkFromBuffer(runtimeptr, buffer, size) !=
            NEURONRUNTIME_NO_ERROR) {
            LOG(ERROR) << "Fail to load Neuron network from buffer";
            return -1;
        }

        auto obj = std::make_shared<RuntimeObject>(runtimeptr);
        obj->SetBoostValue(boostValue);
        pool->Append(std::move(obj));
    }

    mRuntimeObjectPools.emplace(mModelNumber, pool);
    return mModelNumber;
}

bool ModelFactory::GetParam(const std::vector<uint32_t>& params, uint32_t opCode, uint32_t* param) {
    auto it = std::find(params.begin(), params.end(), opCode);
    if (it == params.end()) {
        return false;
    }
    *param = params[it - params.begin() + 1];
    LOG(DEBUG) << "Found OpCode: " << opCode << " with param: " << *param;
    return true;
}

uint32_t ModelFactory::GetPreparedModelSize(int32_t modelId) {
    std::unordered_map<int32_t, std::shared_ptr<ModelInfo>>::const_iterator it =
        mModelInfos.find(modelId);
    if (it == mModelInfos.end()) {
        LOG(ERROR) << "Can not get prepared model size with an invalid model id: " << modelId;
        return 0;
    }

    LOG(DEBUG) << "GetPreparedModelSize, model id:" << modelId
               << " size: " << it->second->GetSize();
    return it->second->GetSize();
}

bool ModelFactory::StorePreparedModel(int32_t modelId, int32_t fd, uint32_t size) {
    UNUSED(modelId);
    UNUSED(fd);
    UNUSED(size);

    LOG(DEBUG) << "StorePreparedModel, model id: " << modelId;
    bool ret = false;

    return ret;
}

void ModelFactory::AsyncReleaseModel(int32_t modelId) {
    NPAGENT_ATRACE_CALL();
    // Lock mutex before release
    std::lock_guard<std::mutex> lock(mMutex);

    std::unordered_map<int32_t, std::shared_ptr<ModelInfo>>::const_iterator it =
        mModelInfos.find(modelId);
    if (it == mModelInfos.end()) {
        LOG(ERROR) << "Can not release model with an invalid model id: " << modelId;
        return;
    }

    it->second->ClearFenceQueue();

    mModelInfos.erase(it);
    mRuntimeObjectPools.erase(modelId);
}

bool ModelFactory::ReleaseModel(int32_t modelId) {
    NPAGENT_ATRACE_CALL();

    // Start a detached thread to release model asynchronously
    std::thread th(&ModelFactory::AsyncReleaseModel, this, modelId);
    th.detach();
    return true;
}

bool ModelFactory::Execute(int32_t modelId, const std::vector<SharedMemory>& inputs,
                           const std::vector<SharedMemory>& outputs) {
    std::unordered_map<int32_t, std::shared_ptr<ModelInfo>>::const_iterator it =
        mModelInfos.find(modelId);
    if (it == mModelInfos.end()) {
        LOG(ERROR) << "Can not execute model with an invalid model id: " << modelId;
        return false;
    }

    // Lock mutex before execution
    std::lock_guard<std::mutex> lock(mMutex);

    LOG(DEBUG) << "Execute, model id: " << modelId;
    auto runtime = const_cast<void*>(reinterpret_cast<const void*>(it->second->GetRuntime()));
    auto boostValue = it->second->GetBoostValue();

    // Process inputs
    for (size_t i = 0; i < inputs.size(); i++) {
        BufferAttribute attribute;
        attribute.ionFd = getFd(inputs[i]);

        auto mapping = map(inputs[i]);
        if (!mapping.has_value()) {
            LOG(ERROR) << "Fail to map input shared memory";
            return false;
        }
        if (!std::holds_alternative<const void*>(mapping.value().pointer)) {
            LOG(ERROR) << "Shoule be constant buffer for inference input";
            return false;
        }

        const void* buffer = std::get<const void*>(mapping.value().pointer);
        LOG(DEBUG) << "Set input[" << i << "], buffer: " << buffer
                   << ", size: " << mapping.value().size;
        if (NeuronRuntime_setInput(runtime, i, buffer, mapping.value().size, attribute) !=
            NEURONRUNTIME_NO_ERROR) {
            LOG(ERROR) << "Fail to set input [" << i << "]";
            return false;
        }
    }

    // Process outputs
    for (size_t i = 0; i < outputs.size(); i++) {
        BufferAttribute attribute;
        attribute.ionFd = getFd(outputs[i]);

        auto mapping = map(outputs[i]);
        if (!mapping.has_value()) {
            LOG(ERROR) << "Fail to map output shared memory";
            return false;
        }
        if (!std::holds_alternative<void*>(mapping.value().pointer)) {
            LOG(ERROR) << "Shoule be writable buffer for inference output";
            return false;
        }

        void* buffer = std::get<void*>(mapping.value().pointer);
        LOG(DEBUG) << "Set output[" << i << "], buffer: " << buffer
                   << ", size: " << mapping.value().size;
        if (NeuronRuntime_setOutput(runtime, i, buffer, mapping.value().size, attribute) !=
            NEURONRUNTIME_NO_ERROR) {
            LOG(ERROR) << "Fail to set output [" << i << "]";
            return false;
        }
    }

    // Set QoS options
    QoSOptions qosOptions = {.preference = NEURONRUNTIME_PREFER_PERFORMANCE,
                             .priority = NEURONRUNTIME_PRIORITY_MED,
                             .boostValue = boostValue,
                             .maxBoostValue = NEURONRUNTIME_BOOSTVALUE_MAX,
                             .minBoostValue = NEURONRUNTIME_BOOSTVALUE_MAX,
                             .deadline = 0,
                             .abortTime = 0,
                             .delayedPowerOffTime = NEURONRUNTIME_POWER_OFF_TIME_DEFAULT,
                             // .RuntimeAPIQoSPowerPolicy = NEURONRUNTIME_POWER_POLICY_DEFAULT,
                             // .RuntimeAPIQoSAppType = NEURONRUNTIME_APP_NORMAL,
                             .profiledQoSData = nullptr};

    if (NeuronRuntime_setQoSOption(runtime, &qosOptions) != NEURONRUNTIME_NO_ERROR) {
        LOG(ERROR) << "Fail to set Qos option";
    }

    LOG(DEBUG) << "Start inference";
    int result = NeuronRuntime_inference(runtime);
    if (result != NEURONRUNTIME_NO_ERROR) {
        LOG(ERROR) << "Inference failed: " << result;
    }
    return (result == NEURONRUNTIME_NO_ERROR);
}

bool ModelFactory::ExecuteFenced(int32_t modelId, const std::vector<SharedMemory>& inputs,
                                 const std::vector<SharedMemory>& outputs,
                                 std::vector<int>& waitForList, int* outputFence) {
    NPAGENT_ATRACE_CALL();
    std::unordered_map<int32_t, std::shared_ptr<RuntimeObjectPool>>::const_iterator it =
        mRuntimeObjectPools.find(modelId);
    if (it == mRuntimeObjectPools.end()) {
        LOG(ERROR) << "Can not execute fenced model with an invalid model id: " << modelId;
        return false;
    }

    auto obj = it->second->Acquire();
    void* runtime = const_cast<void*>(reinterpret_cast<const void*>(obj->GetRuntime()));
    auto boostValue = GetBoostValueOfModel(modelId);

    // Lock mutex before execution
    std::lock_guard<std::mutex> lock(mMutex);

    LOG(DEBUG) << "ExecuteFenced, model id: " << modelId;
    uint8_t fenceSupported = 0;
    if ((NeuronRuntime_isFenceSupported(runtime, &fenceSupported) != NEURONRUNTIME_NO_ERROR) ||
        !fenceSupported) {
        LOG(ERROR) << "The given Neuron runtime instance does not support fence";
        return false;
    }

    // Set QoS options
    QoSOptions qosOptions = {.preference = NEURONRUNTIME_PREFER_PERFORMANCE,
                             .priority = NEURONRUNTIME_PRIORITY_MED,
                             .boostValue = boostValue,
                             .maxBoostValue = NEURONRUNTIME_BOOSTVALUE_MAX,
                             .minBoostValue = NEURONRUNTIME_BOOSTVALUE_MAX,
                             .deadline = 0,
                             .abortTime = 0,
                             .delayedPowerOffTime = NEURONRUNTIME_POWER_OFF_TIME_DEFAULT,
                             // .RuntimeAPIQoSPowerPolicy = NEURONRUNTIME_POWER_POLICY_DEFAULT,
                             // .RuntimeAPIQoSAppType = NEURONRUNTIME_APP_NORMAL,
                             .profiledQoSData = nullptr};

    if (NeuronRuntime_setQoSOption(runtime, &qosOptions) != NEURONRUNTIME_NO_ERROR) {
        LOG(ERROR) << "Fail to set Qos option";
    }
    // Process inputs
    for (size_t i = 0; i < inputs.size(); i++) {
        NPAGENT_ATRACE_NAME("NeuronRuntime_setInput");
        BufferAttribute attribute;
        attribute.ionFd = getFd(inputs[i]);

        auto mapping = map(inputs[i]);
        if (!mapping.has_value()) {
            LOG(ERROR) << "Fail to map input shared memory";
            return false;
        }
        if (!std::holds_alternative<const void*>(mapping.value().pointer)) {
            LOG(ERROR) << "Shoule be constant buffer for inference input";
            return false;
        }

        const void* buffer = std::get<const void*>(mapping.value().pointer);
        // LOG(DEBUG) << "Set input[" << i << "], size: " << mapping.value().size;
        if (NeuronRuntime_setInput(runtime, i, buffer, mapping.value().size, attribute) !=
            NEURONRUNTIME_NO_ERROR) {
            LOG(ERROR) << "Fail to set input [" << i << "]";
            return false;
        }

        if (mDumpInput) {
            auto millisecSinceEpoch = duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count();
            auto dumpPath = kInputDumpPrefix + std::to_string(millisecSinceEpoch);
            std::string inputData(const_cast<char*>(reinterpret_cast<const char*>(buffer)),
                                  mapping.value().size);
            if (android::base::WriteStringToFile(inputData, dumpPath, 0660, getuid(), getgid())) {
                LOG(DEBUG) << "Dump input to " << dumpPath << " size: " << mapping.value().size;
            } else {
                LOG(ERROR) << "Fail to dump input";
            }
        }
    }

    // Process outputs
    for (size_t i = 0; i < outputs.size(); i++) {
        NPAGENT_ATRACE_NAME("NeuronRuntime_setOutput");
        BufferAttribute attribute;
        attribute.ionFd = getFd(outputs[i]);

        auto mapping = map(outputs[i]);
        if (!mapping.has_value()) {
            LOG(ERROR) << "Fail to map output shared memory";
            return false;
        }
        if (!std::holds_alternative<void*>(mapping.value().pointer)) {
            LOG(ERROR) << "Shoule be writable buffer for inference output";
            return false;
        }

        void* buffer = std::get<void*>(mapping.value().pointer);
        // LOG(DEBUG) << "Set output[" << i << "], size: " << mapping.value().size;

        if (NeuronRuntime_setOutput(runtime, i, buffer, mapping.value().size, attribute) !=
            NEURONRUNTIME_NO_ERROR) {
            LOG(ERROR) << "Fail to set output [" << i << "]";
            return false;
        }
    }

    FenceInfo* fenceInfo = new FenceInfo();
    // TODO: Merge multiple input fence fds
    int waitFd = (waitForList.size() > 0 ? waitForList[0] : 0);
    fenceInfo->inputFenceFd = dup(waitFd);
    if (fenceInfo->inputFenceFd < 0) {
        LOG(ERROR) << "Fail to dup inputFenceFd";
        delete fenceInfo;
        return false;
    }

    LOG(DEBUG) << "Dup inputFenceFd: " << waitFd << " to " << fenceInfo->inputFenceFd;
    LOG(DEBUG) << "Start inferenceFenced, inputFenceFd: " << fenceInfo->inputFenceFd;
    int result = NEURONRUNTIME_NO_ERROR;
    {
        LOG(DEBUG) << "Inference fenced with Neuron runtime: " << std::hex << runtime;
        NPAGENT_ATRACE_NAME("NeuronRuntime_inferenceFenced");
        result = NeuronRuntime_inferenceFenced(runtime, fenceInfo);
    }
    if (result == NEURONRUNTIME_NO_ERROR) {
        *outputFence = fenceInfo->fenceFd;

        std::unordered_map<int32_t, std::shared_ptr<ModelInfo>>::const_iterator it =
            mModelInfos.find(modelId);
        if (it != mModelInfos.end()) {
            it->second->EnqueueFence(fenceInfo->fenceFd);
        }
    } else {
        LOG(ERROR) << "InferenceFenced failed: " << result;
    }
    close(fenceInfo->inputFenceFd);
    delete fenceInfo;
    it->second->Release(obj);
    if (mDumpOutput) {
        auto fence = neuropilotagent::SyncFence::Create(android::base::unique_fd(*outputFence));
        auto r = fence.SyncWait({/* no timeout */});
        if (r != neuropilotagent::SyncFence::FenceState::SIGNALED) {
            LOG(ERROR) << "syncWait failed, fd: " << fence.GetFd()
                       << ", state: " << static_cast<uint8_t>(r);
        } else {
            LOG(DEBUG) << "Wait outputFenceFd: " << *outputFence;
            auto millisecSinceEpoch = duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count();
            auto dumpPath = kOutputDumpPrefix + std::to_string(millisecSinceEpoch);
            auto mapping = map(outputs[0]);
            if (mapping.has_value() && std::holds_alternative<void*>(mapping.value().pointer)) {
                void* buffer = std::get<void*>(mapping.value().pointer);
                std::string outputData(const_cast<char*>(reinterpret_cast<const char*>(buffer)),
                                       mapping.value().size);
                if (android::base::WriteStringToFile(outputData, dumpPath, 0660, getuid(),
                                                     getgid())) {
                    LOG(DEBUG) << "Dump output to " << dumpPath;
                } else {
                    LOG(ERROR) << "Fail to dump output";
                }
            }
        }
        *outputFence = dup(*outputFence);
    }
    return (result == NEURONRUNTIME_NO_ERROR);
}

std::optional<Attributes> ModelFactory::GetAttributes(int32_t modelId) {
    LOG(WARNING) << "[Deprecated]GetAttributes, model id: " << modelId;
    return std::nullopt;
}

std::vector<uint32_t> ModelFactory::GetParams(int32_t modelId) {
    NPAGENT_ATRACE_CALL();
    LOG(DEBUG) << "GetParams, model id: " << modelId;
    std::unordered_map<int32_t, std::shared_ptr<ModelInfo>>::const_iterator it =
        mModelInfos.find(modelId);
    if (it == mModelInfos.end()) {
        LOG(ERROR) << "Fail to get model params with an invalid model id:" << modelId;
        return {};
    }

    return it->second->GetParams();
}

bool ModelFactory::SetParams(int32_t modelId, const std::vector<uint32_t>& params) {
    NPAGENT_ATRACE_CALL();
    std::unordered_map<int32_t, std::shared_ptr<ModelInfo>>::const_iterator it =
        mModelInfos.find(modelId);
    if (it == mModelInfos.end()) {
        LOG(ERROR) << "Can not execute model with an invalid model id: " << modelId;
        return false;
    }

    // Lock mutex before updating parameter
    std::lock_guard<std::mutex> lock(mMutex);

    LOG(DEBUG) << "SetParams, model id: " << modelId;
    uint32_t param = 0;

    if (GetParam(params, OP_BOOST_VALUE, &param)) {
        LOG(DEBUG) << "Get boost value from param: " << param;
        it->second->SetBoostValue(static_cast<uint8_t>(param));
        LOG(DEBUG) << "Updated boost value: " << static_cast<uint32_t>(it->second->GetBoostValue());
    }

    return true;
}

uint8_t ModelFactory::GetBoostValueOfModel(int32_t modelId) {
    NPAGENT_ATRACE_CALL();
    std::unordered_map<int32_t, std::shared_ptr<ModelInfo>>::const_iterator it =
        mModelInfos.find(modelId);
    if (it == mModelInfos.end()) {
        LOG(ERROR) << "Can not execute model with an invalid model id: " << modelId;
        return false;
    }

    // Lock mutex before getting boost value
    std::lock_guard<std::mutex> lock(mMutex);
    auto boostValue = it->second->GetBoostValue();
    LOG(DEBUG) << "GetBoostValueOfModel, model id: " << modelId
               << " boost value: " << static_cast<uint32_t>(boostValue);
    return boostValue;
}

}  // namespace neuropilotagent
}  // namespace mediatek
