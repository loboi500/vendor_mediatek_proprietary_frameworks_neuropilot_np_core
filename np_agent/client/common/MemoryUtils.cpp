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

#define LOG_TAG "NpAgentMemoryUtils"

#include "MemoryUtils.h"

#include <android-base/logging.h>
#include <ui/gralloc_extra.h>
#include <vndk/hardware_buffer.h>

namespace mediatek {
namespace neuropilotagent {

hidl_memory createHidlMemoryFromAHardwareBuffer(AHardwareBuffer* ahwb) {
    AHardwareBuffer_Desc bufferDesc = {};
    AHardwareBuffer_describe(ahwb, &bufferDesc);
    const native_handle_t* handle = AHardwareBuffer_getNativeHandle(ahwb);
    return hidl_memory("hardware_buffer_blob", handle, bufferDesc.width);
}

void releaseHidlMemoryFromAHardwareBuffer(const hidl_memory& memory) {
    int fd = memory.handle()->data[0];
    close(fd);
}

hidl_memory createHidlMemoryFromFd(int32_t fd, uint32_t size) {
    native_handle_t* nativeHandle = native_handle_create(1, 0);
    if (nativeHandle != nullptr) {
        nativeHandle->data[0] = dup(fd);
        LOG(DEBUG) << "dup fd " << nativeHandle->data[0] << " from fd:" << fd;
    }
    android::hardware::hidl_handle hidlHandle;
    hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
    return hidl_memory("ion_fd", std::move(hidlHandle), size);
}

void releaseHidlMemoryFromFd(const hidl_memory& memory) {
    int fd = memory.handle()->data[0];
    LOG(DEBUG) << "close fd " << fd;
    close(fd);
}

void getFdInfo(int fd, std::ostringstream* ss) {
    static pid_t pid = getpid();
    *ss << "fdInfo: fd(" << fd << ") ";
    if (fd < 0) {
        *ss << "failed to get fd info" << std::endl;
        return;
    }

    std::ostringstream dir_path;
    dir_path << "/proc/" << pid << "/fd/" << fd;
    char buf[1024] = {0};
    ssize_t res = readlink(dir_path.str().c_str(), buf, sizeof(buf) - 1);
    if (res < 0) {
        *ss << "failed to read link" << std::endl;
    } else {
        buf[res] = '\0';
        *ss << "link= " << buf << std::endl;
    }
}

}  // namespace neuropilotagent
}  // namespace mediatek
