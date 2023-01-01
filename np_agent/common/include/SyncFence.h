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

#pragma once

#include <android-base/unique_fd.h>

#include <chrono>
#include <memory>
#include <optional>

using Handle = android::base::unique_fd;
using SharedHandle = std::shared_ptr<const Handle>;

namespace mediatek {
namespace neuropilotagent {

// Representation of sync_fence.
class SyncFence {
public:
    static SyncFence CreateAsSignaled();
    static SyncFence Create(android::base::unique_fd fd);
    static SyncFence Create(SharedHandle syncFence);

    // The function syncWait() has the same semantics as the system function
    // ::sync_wait(), except that the syncWait() return value is semantically
    // richer.
    enum class FenceState {
        ACTIVE,    // fence has not been signaled
        SIGNALED,  // fence has been signaled
        ERROR,     // fence has been placed in the error state
        UNKNOWN,   // either bad argument passed to syncWait(), or internal error
    };
    using Timeout = std::chrono::duration<int, std::milli>;
    using OptionalTimeout = std::optional<Timeout>;

    FenceState SyncWait(OptionalTimeout optionalTimeout) const;

    SharedHandle GetSharedHandle() const;
    bool HasFd() const;
    int GetFd() const;

private:
    explicit SyncFence(SharedHandle syncFence);

    SharedHandle mSyncFence;
};

}  // namespace neuropilotagent
}  // namespace mediatek
