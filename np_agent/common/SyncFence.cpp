/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "NpAgentSyncFence"

#include "SyncFence.h"

#include <android-base/logging.h>
#include <errno.h>
#include <poll.h>

namespace mediatek {
namespace neuropilotagent {

SyncFence SyncFence::CreateAsSignaled() { return SyncFence(nullptr); }

SyncFence SyncFence::Create(android::base::unique_fd fd) {
    CHECK(fd.ok());
    return SyncFence(std::make_shared<const Handle>(std::move(fd)));
}

SyncFence SyncFence::Create(SharedHandle syncFence) { return SyncFence(std::move(syncFence)); }

SyncFence::SyncFence(SharedHandle syncFence) : mSyncFence(std::move(syncFence)) {}

SyncFence::FenceState SyncFence::SyncWait(OptionalTimeout optionalTimeout) const {
    if (mSyncFence == nullptr) {
        return FenceState::SIGNALED;
    }

    const int fd = mSyncFence->get();
    const int timeout = optionalTimeout.value_or(Timeout{-1}).count();

    // This implementation is directly based on the ::sync_wait() implementation.

    struct pollfd fds {
        .fd = fd, .events = POLLIN, .revents = 0
    };
    int ret;

    if (fd < 0) {
        errno = EINVAL;
        return FenceState::UNKNOWN;
    }

    do {
        ret = poll(&fds, 1, timeout);
        if (ret > 0) {
            if (fds.revents & POLLNVAL) {
                errno = EINVAL;
                return FenceState::UNKNOWN;
            }
            if (fds.revents & POLLERR) {
                errno = EINVAL;
                return FenceState::ERROR;
            }
            return FenceState::SIGNALED;
        } else if (ret == 0) {
            errno = ETIME;
            return FenceState::ACTIVE;
        }
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    return FenceState::UNKNOWN;
}

SharedHandle SyncFence::GetSharedHandle() const { return mSyncFence; }

bool SyncFence::HasFd() const { return mSyncFence != nullptr; }

int SyncFence::GetFd() const { return mSyncFence == nullptr ? -1 : mSyncFence->get(); }

}  // namespace neuropilotagent
}  // namespace mediatek
