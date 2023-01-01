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

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "Executor.h"
#include "NpAgentTypes.h"

namespace mediatek {
namespace neuropilotagent {

class ModelAttributes {
public:
    explicit ModelAttributes(int32_t modelId);

    bool GetInputFormat(uint32_t* value) { return GetParam(OP_INPUT_FORMAT, value); }

    bool GetOutputFormat(uint32_t* value) { return GetParam(OP_OUTPUT_FORMAT, value); }

    bool GetInputCompressionMode(uint32_t* value) { return GetParam(OP_INPUT_COMPRESSION, value); }

    bool GetOutputCompressionMode(uint32_t* value) {
        return GetParam(OP_OUTPUT_COMPRESSION, value);
    }

    bool GetInputStride(uint32_t* value) { return GetParam(OP_INPUT_W_STRIDE, value); }

    bool GetOutputStride(uint32_t* value) { return GetParam(OP_OUTPUT_W_STRIDE, value); }

    bool GetInputHeight(uint32_t* value) { return GetParam(OP_INPUT_HEIGHT, value); }

    bool GetInputWidth(uint32_t* value) { return GetParam(OP_INPUT_WIDTH, value); }

    bool GetOutputHeight(uint32_t* value) { return GetParam(OP_OUTPUT_HEIGHT, value); }

    bool GetOutputWidth(uint32_t* value) { return GetParam(OP_OUTPUT_WIDTH, value); }

    uint32_t GetBufferFormat() { return 0; }

    uint32_t GetBufferCompressionMode() { return 0; }

    bool IsValid() { return mValid; }

private:
    bool GetParam(uint32_t opCode, uint32_t* param);

private:
    bool mValid = false;
    std::vector<uint32_t> mParams;
};

}  // namespace neuropilotagent
}  // namespace mediatek
