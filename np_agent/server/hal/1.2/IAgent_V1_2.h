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
/* MediaTek Inc. (C) 2021. All rights reserved.
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

#include <android/log.h>

#include <android-base/macros.h>
#include <memory>
#include <optional>

#include "HalInterfaces.h"
#include "hal/1.0/IAgentImpl_V1_0.h"
#include "hal/1.1/IAgentImpl_V1_1.h"
#include "hal/1.2/IAgentImpl_V1_2.h"

namespace mediatek {
namespace neuropilotagent {

class IAgent_V1_2 : public IAgent {
public:
    Return<int32_t> prepareModel(const hidl_memory& model, const Options& options) override {
        return mV10Impl->prepareModel(model, options);
    }

    Return<int32_t> prepareModelFromCache(const hidl_memory& cache,
                                          const Options& options) override {
        return mV10Impl->prepareModelFromCache(cache, options);
    }

    Return<uint32_t> getPreparedModelSize(int32_t modelId) override {
        return mV10Impl->getPreparedModelSize(modelId);
    }

    Return<ErrorStatus> storePreparedModel(int32_t modelId, const hidl_memory& buffer) override {
        return mV10Impl->storePreparedModel(modelId, buffer);
    }

    Return<ErrorStatus> releaseModel(int32_t modelId) override {
        return mV10Impl->releaseModel(modelId);
    }

    Return<ErrorStatus> execute(int32_t modelId, const hidl_vec<hidl_memory>& inputs,
                                const hidl_vec<hidl_memory>& outputs) override {
        return mV10Impl->execute(modelId, inputs, outputs);
    }

    Return<void> executeFenced(int32_t modelId, const hidl_vec<hidl_memory>& inputs,
                               const hidl_vec<hidl_memory>& outputs,
                               const hidl_vec<hidl_handle>& waitFor,
                               executeFenced_cb _hidl_cb) override {
        int outputFence = -1;
        ErrorStatus status =
            mV10Impl->executeFenced(modelId, inputs, outputs, waitFor, &outputFence);
        if (status == ErrorStatus::NONE && outputFence != -1) {
            native_handle_t* nativeHandle = native_handle_create(1, 1);
            if (nativeHandle == nullptr) {
                _hidl_cb(ErrorStatus::GENERAL_FAILURE, hidl_handle(nullptr), nullptr);
                return Void();
            }
            nativeHandle->data[0] = outputFence;
            hidl_handle hidlHandle;
            hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
            sp<IAgentFencedExecutionCallbackImpl_V1_0> fencedCallback =
                new IAgentFencedExecutionCallbackImpl_V1_0(modelId, outputFence);
            _hidl_cb(ErrorStatus::NONE, hidlHandle, fencedCallback);
            return Void();
        }
        _hidl_cb(ErrorStatus::GENERAL_FAILURE, hidl_handle(nullptr), nullptr);
        return Void();
    }

    Return<void> getAttributes(int32_t modelId, getAttributes_cb _hidl_cb) override {
        std::optional<Attributes> attributes = mV10Impl->getAttributes(modelId);
        if (attributes) {
            _hidl_cb(ErrorStatus::NONE, *attributes);
        } else {
            Attributes ret = {0};
            _hidl_cb(ErrorStatus::GENERAL_FAILURE, ret);
        }
        return Void();
    }

    Return<int32_t> prepareModelFromCache_1_1(const hidl_memory& cache,
                                              const hidl_vec<uint32_t>& params) override {
        return mV11Impl->prepareModelFromCache(cache, params);
    }

    Return<void> getParams(int32_t modelId, getParams_cb _hidl_cb) override {
        auto params = mV11Impl->getParams(modelId);
        _hidl_cb(ErrorStatus::NONE, params);
        return Void();
    }

    Return<ErrorStatus> setParams(int32_t modelId, const hidl_vec<uint32_t>& params) override {
        return mImpl->setParams(modelId, params);
    }

    Return<void> executeFenced_1_2(int32_t modelId, const hidl_vec<hidl_memory>& inputs,
                                   const hidl_vec<hidl_memory>& outputs,
                                   const hidl_vec<hidl_handle>& waitFor,
                                   const hidl_vec<uint32_t>& params,
                                   executeFenced_cb _hidl_cb) override {
        UNUSED(params);
        int outputFence = -1;
        ErrorStatus status =
            mV10Impl->executeFenced(modelId, inputs, outputs, waitFor, &outputFence);
        if (status == ErrorStatus::NONE && outputFence != -1) {
            native_handle_t* nativeHandle = native_handle_create(1, 1);
            if (nativeHandle == nullptr) {
                _hidl_cb(ErrorStatus::GENERAL_FAILURE, hidl_handle(nullptr), nullptr);
                return Void();
            }
            nativeHandle->data[0] = outputFence;
            hidl_handle hidlHandle;
            hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
            sp<IAgentFencedExecutionCallbackImpl_V1_0> fencedCallback =
                new IAgentFencedExecutionCallbackImpl_V1_0(modelId, outputFence);
            _hidl_cb(ErrorStatus::NONE, hidlHandle, fencedCallback);
            return Void();
        }
        _hidl_cb(ErrorStatus::GENERAL_FAILURE, hidl_handle(nullptr), nullptr);
        return Void();
    }

protected:
    std::unique_ptr<IAgentImpl_V1_2> mImpl = std::make_unique<IAgentImpl_V1_2>();
    std::unique_ptr<IAgentImpl_V1_1> mV11Impl = std::make_unique<IAgentImpl_V1_1>();
    std::unique_ptr<IAgentImpl_V1_0> mV10Impl = std::make_unique<IAgentImpl_V1_0>();
};

}  // namespace neuropilotagent
}  // namespace mediatek
