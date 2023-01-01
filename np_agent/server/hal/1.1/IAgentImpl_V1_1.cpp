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

#define LOG_TAG "IAgentImpl_V1_1"

#include "IAgentImpl_V1_1.h"

#include <android-base/logging.h>
#include <sys/mman.h>
#include <ui/gralloc_extra.h>

#include <vector>

#include "ModelFactory.h"
#include "SharedMemory.h"
#include "Trace.h"
#include "Utils.h"

namespace mediatek {
namespace neuropilotagent {

Return<int32_t> IAgentImpl_V1_1::prepareModelFromCache(const hidl_memory& hidlMemory,
                                                       const hidl_vec<uint32_t>& params) {
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

    return ModelFactory::GetInstance().PrepareModelFromCache(fd, size, params);
}

std::vector<uint32_t> IAgentImpl_V1_1::getParams(int32_t modelId) {
    return ModelFactory::GetInstance().GetParams(modelId);
}

}  // namespace neuropilotagent
}  // namespace mediatek
