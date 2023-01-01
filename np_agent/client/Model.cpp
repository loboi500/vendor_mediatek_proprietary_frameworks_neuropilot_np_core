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

#define LOG_TAG "NpAgentModel"

#include "Model.h"

#include <android-base/logging.h>

#include "HalInterfaces.h"
#include "ServiceManager.h"

namespace mediatek {
namespace neuropilotagent {

int32_t Model::Build() {
    LOG(DEBUG) << "Build";
    mFinish = true;

    return ServiceManager::GetInstance()->PrepareModelFromCache(mSourceFd, mSourceSize,
                                                                kParams.ToVector());
}

void Model::Release(int32_t modelId) {
    LOG(DEBUG) << "Release";
    return ServiceManager::GetInstance()->ReleaseModel(modelId);
}

bool Model::Inference(int32_t modelId, const Executor& executor) {
    LOG(DEBUG) << "Inference";
    return ServiceManager::GetInstance()->Execute(modelId, executor.GetInputs(),
                                                  executor.GetInputSizes(), executor.GetOutputs(),
                                                  executor.GetOutputSizes());
}

bool Model::InferenceFenced(int32_t modelId, const Executor& executor, std::vector<int> inputFence,
                            int* outputFence) {
    LOG(DEBUG) << "InferenceFenced";
    return ServiceManager::GetInstance()->ExecuteFenced(
        modelId, executor.GetInputs(), executor.GetInputSizes(), executor.GetOutputs(),
        executor.GetOutputSizes(), inputFence, outputFence);
}

uint32_t Model::GetCompilationSize(int32_t modelId) {
    return ServiceManager::GetInstance()->GetPreparedModelSize(modelId);
}

bool Model::StorePreparedModel(int32_t modelId, void* buffer, size_t size) {
    return ServiceManager::GetInstance()->StorePreparedModel(modelId, buffer, size);
}

bool Model::Update(int32_t modelId, const ModelParams& params) {
    LOG(DEBUG) << "Update model id: " << modelId;
    for (auto const& pair : params.GetParams()) {
        LOG(DEBUG) << "[" << pair.first << ": " << pair.second << "]\n";
    }
    return ServiceManager::GetInstance()->SetParams(modelId, params.ToVector());
}

}  // namespace neuropilotagent
}  // namespace mediatek
