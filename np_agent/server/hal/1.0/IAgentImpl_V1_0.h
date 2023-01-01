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

#include <android/log.h>

#include <memory>
#include <optional>

#include "HalInterfaces.h"
#include "options/RuntimeOptions.h"

namespace mediatek {
namespace neuropilotagent {

class IAgentImpl_V1_0 {
public:
    IAgentImpl_V1_0() = default;

    ~IAgentImpl_V1_0() {}

public:
    Return<int32_t> prepareModel(const hidl_memory& hidlMemory, const Options& options);

    Return<int32_t> prepareModelFromCache(const hidl_memory& hidlMemory, const Options& options);

    Return<uint32_t> getPreparedModelSize(int32_t modelId);

    Return<ErrorStatus> storePreparedModel(int32_t modelId, const hidl_memory& hidlMemory);

    Return<ErrorStatus> releaseModel(int32_t modelId);

    Return<ErrorStatus> execute(int32_t modelId, const hidl_vec<hidl_memory>& inputs,
                                const hidl_vec<hidl_memory>& outputs);

    Return<ErrorStatus> executeFenced(int32_t modelId, const hidl_vec<hidl_memory>& inputs,
                                      const hidl_vec<hidl_memory>& outputs,
                                      const hidl_vec<hidl_handle>& waitFor, int* outputFence);

    std::optional<Attributes> getAttributes(int32_t modelId);

private:
    bool mBindCpuLittleCores = RuntimeOptions().BindCpuLittleCores();
};

class IAgentFencedExecutionCallbackImpl_V1_0 : public IFencedExecutionCallback {
public:
    explicit IAgentFencedExecutionCallbackImpl_V1_0(int32_t modelId, int fenceFd)
        : mModelId(modelId), mFenceFd(fenceFd) {}

    Return<ErrorStatus> getExecutionInfo() override;

private:
    int32_t mModelId;
    int mFenceFd;
};

}  // namespace neuropilotagent
}  // namespace mediatek
