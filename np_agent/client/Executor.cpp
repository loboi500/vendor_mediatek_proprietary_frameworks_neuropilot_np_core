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

#define LOG_TAG "NpAgentExecutor"

#include "Executor.h"

#include <android-base/logging.h>

namespace mediatek {
namespace neuropilotagent {

void Executor::SetInputs(std::vector<int> inputs, std::vector<uint32_t> inputSizes) {
    for (auto i = 0; i < inputs.size(); i++) {
        int dupFd = dup(inputs[i]);
        LOG(DEBUG) << "SetInputs, dup fd " << dupFd << " from " << inputs[i];
        mInputs.push_back(dupFd);
    }
    mInputSizes = inputSizes;
}

void Executor::SetOutputs(std::vector<int> outputs, std::vector<uint32_t> outputSizes) {
    for (auto i = 0; i < outputs.size(); i++) {
        int dupFd = dup(outputs[i]);
        LOG(DEBUG) << "SetOutputs, dup fd " << dupFd << " from " << outputs[i];
        mOutputs.push_back(dupFd);
    }
    mOutputSizes = outputSizes;
}

Executor::~Executor() {
    for (auto i = 0; i < mInputs.size(); i++) {
        LOG(DEBUG) << "close input fd " << mInputs[i];
        close(mInputs[i]);
    }
    for (auto i = 0; i < mOutputs.size(); i++) {
        LOG(DEBUG) << "close output fd " << mOutputs[i];
        close(mOutputs[i]);
    }
    if (mWaitForFd != -1) {
        LOG(DEBUG) << "close waitFor fd " << mWaitForFd;
        close(mWaitForFd);
    }
}

}  // namespace neuropilotagent
}  // namespace mediatek
