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

#pragma once

#include <android-base/logging.h>
#include <android-base/macros.h>

#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Fence.h"
#include "HalInterfaces.h"
#include "NpAgentTypes.h"
#include "RuntimeAPI.h"
#include "SharedMemory.h"
#include "SyncFence.h"
#include "Utils.h"

namespace mediatek {
namespace neuropilotagent {

class ModelInfo {
public:
    static constexpr uint32_t kFenceQueueSize = 10;

public:
    explicit ModelInfo(void* runtimePtr, uint32_t size, std::vector<uint32_t> modelParams)
        : mRuntime(runtimePtr), mDlaSize(size), mModelParams(modelParams) {
        auto it = std::find(mModelParams.begin(), mModelParams.end(), OP_BOOST_VALUE);
        if (it != mModelParams.end()) {
            mBoostValue = static_cast<uint8_t>(mModelParams[it - mModelParams.begin() + 1]);
            LOG(DEBUG) << "Initial boost value: " << static_cast<uint32_t>(mBoostValue);
        }
    }

    ~ModelInfo();

    const void* GetRuntime() const { return mRuntime; }

    void SetBoostValue(uint8_t value) { mBoostValue = value; }

    uint8_t GetBoostValue() const { return mBoostValue; }

    uint32_t GetSize() { return mDlaSize; }

    const std::vector<uint32_t>& GetParams() const { return mModelParams; }

    void EnqueueFence(int fd);

    void ClearFenceQueue();

private:
    void* mRuntime;
    uint32_t mDlaSize;
    uint8_t mBoostValue = NEURONRUNTIME_BOOSTVALUE_MAX;
    utils::FixedQueue<SyncFence, kFenceQueueSize>
        mFenceQueue;  // Keep the last 10 dupped inference done fence fds
    std::vector<uint32_t> mModelParams;
};

class RuntimeObject {
public:
    RuntimeObject() {}

    explicit RuntimeObject(void* runtimePtr) : mRuntime(runtimePtr) {}

    void SetBoostValue(uint8_t boostValue) { mBoostValue = boostValue; }

    ~RuntimeObject();

    const void* GetRuntime() const { return mRuntime; }

    uint8_t GetBoostValue() const { return mBoostValue; }

private:
    void* mRuntime;
    uint8_t mBoostValue = NEURONRUNTIME_BOOSTVALUE_MAX;
    std::unordered_map<std::size_t, FenceInfo*> mFenceInfos;
};

class RuntimeObjectPool {
public:
    RuntimeObjectPool() {}

    void Append(std::shared_ptr<RuntimeObject> object) { mObjects.push_back(object); }

    std::shared_ptr<RuntimeObject> Acquire() {
        auto obj = std::move(mObjects.front());
        mObjects.pop_front();
        return std::move(obj);
    }

    void Release(std::shared_ptr<RuntimeObject> object) { mObjects.push_back(object); }

private:
    std::list<std::shared_ptr<RuntimeObject>> mObjects;
};

class ModelFactory {
public:
    static constexpr uint32_t kDefaultPoolSize = 1;

public:
    static ModelFactory& GetInstance() {
        static ModelFactory gInstance;  // Instantiated when this function is called
        return gInstance;
    }

    int32_t PrepareModel(int32_t fd, uint32_t size, uint32_t stride);

    int32_t PrepareModelFromCache(int32_t fd, uint32_t size, const Options& options);

    int32_t PrepareModelFromCache(int32_t fd, uint32_t size, const std::vector<uint32_t>& params);

    uint32_t GetPreparedModelSize(int32_t modelId);

    bool StorePreparedModel(int32_t modelId, int32_t fd, uint32_t size);

    bool ReleaseModel(int32_t modelId);

    bool Execute(int32_t modelId, const std::vector<SharedMemory>& inputs,
                 const std::vector<SharedMemory>& outputs);

    bool ExecuteFenced(int32_t modelId, const std::vector<SharedMemory>& inputs,
                       const std::vector<SharedMemory>& outputs, std::vector<int>& waitFor,
                       int* outputFence);

    std::optional<Attributes> GetAttributes(int32_t modelId);

    std::vector<uint32_t> GetParams(int32_t modelId);

    bool SetParams(int32_t modelId, const std::vector<uint32_t>& params);

    void AsyncReleaseModel(int32_t modelId);

private:
    std::atomic<int32_t> mModelNumber = 0;
    std::unordered_map<int32_t, std::shared_ptr<ModelInfo>> mModelInfos;
    std::unordered_map<int32_t, std::shared_ptr<RuntimeObjectPool>> mRuntimeObjectPools;
    std::mutex mMutex;
    bool mDumpInput = false;
    bool mDumpOutput = false;
    std::string kInputDumpPrefix = "/data/vendor/nn/input_dump_";
    std::string kOutputDumpPrefix = "/data/vendor/nn/output_dump_";

private:
    ModelFactory();

    ~ModelFactory();

    bool GetParam(const std::vector<uint32_t>& params, uint32_t opCode, uint32_t* param);

    uint8_t GetBoostValueOfModel(int32_t modelId);

    DISALLOW_COPY_AND_ASSIGN(ModelFactory);
};

}  // namespace neuropilotagent
}  // namespace mediatek
