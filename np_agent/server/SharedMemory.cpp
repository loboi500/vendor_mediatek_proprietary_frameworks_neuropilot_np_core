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

#define LOG_TAG "SharedMemory"

#include "SharedMemory.h"

#include <android-base/logging.h>
#include <android-base/mapped_file.h>
#include <android-base/scopeguard.h>

#include <any>
#include <memory>
#include <utility>

namespace mediatek {
namespace neuropilotagent {

struct MmapFdMappingContext {
    int prot;
    std::any context;
};

GeneralResult<android::base::unique_fd> dupFd(int fd) {
    if (fd < 0) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "dupFd was passed an invalid fd";
    }
    auto uniqueFd = android::base::unique_fd(dup(fd));
    if (!uniqueFd.ok()) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to dup the fd";
    }
    return uniqueFd;
}

size_t getSize(const Memory::Ashmem& memory) { return memory.size; }

size_t getSize(const Memory::Fd& memory) { return memory.size; }

int32_t getFd(const Memory::Ashmem& memory) { return memory.fd; }

int32_t getFd(const Memory::Fd& memory) { return memory.fd; }

GeneralResult<SharedMemory> createSharedMemoryFromUniqueFd(size_t size, int prot,
                                                           android::base::unique_fd fd,
                                                           size_t offset) {
    auto handle = Memory::Fd{
        .size = size,
        .prot = prot,
        .fd = std::move(fd),
        .offset = offset,
    };
    return std::make_shared<const Memory>(Memory{.handle = std::move(handle)});
}

GeneralResult<Mapping> map(const Memory::Ashmem& memory) {
    UNUSED(memory);
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Cannot map ashmem memory";
}

GeneralResult<Mapping> map(const Memory::Fd& memory) {
    std::shared_ptr<android::base::MappedFile> mapping =
        android::base::MappedFile::FromFd(memory.fd, memory.offset, memory.size, memory.prot);
    if (mapping == nullptr) {
        return NN_ERROR() << "Can't mmap the file descriptor.";
    }
    char* data = mapping->data();

    const bool writable = (memory.prot & PROT_WRITE) != 0;
    std::variant<const void*, void*> pointer;
    if (writable) {
        pointer = static_cast<void*>(data);
    } else {
        pointer = static_cast<const void*>(data);
    }

    auto context = MmapFdMappingContext{.prot = memory.prot, .context = std::move(mapping)};
    return Mapping{.pointer = pointer, .size = memory.size, .context = std::move(context)};
}

GeneralResult<SharedMemory> createSharedMemoryFromFd(size_t size, int prot, int fd, size_t offset) {
    return createSharedMemoryFromUniqueFd(size, prot, NN_TRY(dupFd(fd)), offset);
}

size_t getSize(const SharedMemory& memory) {
    CHECK(memory != nullptr);
    return std::visit([](const auto& x) { return getSize(x); }, memory->handle);
}

int32_t getFd(const SharedMemory& memory) {
    CHECK(memory != nullptr);
    return std::visit([](const auto& x) { return getFd(x); }, memory->handle);
}

GeneralResult<Mapping> map(const SharedMemory& memory) {
    if (memory == nullptr) {
        return NN_ERROR() << "Unable to map nullptr SharedMemory object";
    }

    return std::visit([](const auto& x) { return map(x); }, memory->handle);
}

bool flush(const Mapping& mapping) {
    if (const auto* mmapFdMapping = std::any_cast<MmapFdMappingContext>(&mapping.context)) {
        if (!std::holds_alternative<void*>(mapping.pointer)) {
            return true;
        }
        void* data = std::get<void*>(mapping.pointer);
        const int prot = mmapFdMapping->prot;
        if (prot & PROT_WRITE) {
            const size_t size = mapping.size;
            return msync(data, size, MS_SYNC) == 0;
        }
    }
    // No-op for other types of memory.
    return false;
}

}  // namespace neuropilotagent
}  // namespace mediatek
