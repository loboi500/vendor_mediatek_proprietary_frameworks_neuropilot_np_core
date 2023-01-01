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

#include "Trace.h"

#include <android-base/macros.h>
#include <dlfcn.h>

#include <type_traits>

#if defined(NN_TARGET_ANDROID)
// Refer to cutils/Trace.h
#define ATRACE_TAG_NNAPI (1 << 25)
#endif

namespace npagent {

ATracerBase::~ATracerBase() {}

#if defined(NN_TARGET_NDK)

class ATracerNdk : public ATracerBase {
public:
    ATracerNdk() {
        mHandle = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
        if (mHandle == nullptr) {
            return;
        }

        mFpAtraceIsEnabled = reinterpret_cast<FpIsEnabled>(dlsym(mHandle, "ATrace_isEnabled"));
        mFpAtraceBeginSection =
            reinterpret_cast<FpBeginSection>(dlsym(mHandle, "ATrace_beginSection"));
        mFpAtraceEndSection = reinterpret_cast<FpEndSection>(dlsym(mHandle, "ATrace_endSection"));
        if (!mFpAtraceIsEnabled || !mFpAtraceBeginSection || !mFpAtraceEndSection) {
            dlclose(mHandle);
            mHandle = nullptr;
        }
    }

    ~ATracerNdk() {
        if (mHandle) {
            dlclose(mHandle);
        }
    }

    void BeginSection(const char* name) override {
        if (mHandle && mFpAtraceIsEnabled()) {
            mFpAtraceBeginSection(name);
        }
    }

    void EndSection() override {
        if (mHandle) {
            mFpAtraceEndSection();
        }
    }

private:
    using FpIsEnabled = std::add_pointer<bool()>::type;
    using FpBeginSection = std::add_pointer<void(const char*)>::type;
    using FpEndSection = std::add_pointer<void()>::type;

    void* mHandle = nullptr;  // Handle to libandroid.so library. Null if not supported.
    FpIsEnabled mFpAtraceIsEnabled = nullptr;
    FpBeginSection mFpAtraceBeginSection = nullptr;
    FpEndSection mFpAtraceEndSection = nullptr;
};

#elif defined(NN_TARGET_ANDROID)

class ATracerAndroid : public ATracerBase {
public:
    ATracerAndroid() {
        mHandle = dlopen("libcutils.so", RTLD_NOW | RTLD_LOCAL);
        if (mHandle == nullptr) {
            return;
        }

        mFpAtraceGetEnabledTags =
            reinterpret_cast<FpGetEnablesTags>(dlsym(mHandle, "atrace_get_enabled_tags"));
        mFpAtraceBeginSection =
            reinterpret_cast<FpBeginSection>(dlsym(mHandle, "atrace_begin_body"));
        mFpAtraceEndSection = reinterpret_cast<FpEndSection>(dlsym(mHandle, "atrace_end_body"));
        if ((!mFpAtraceGetEnabledTags || !mFpAtraceBeginSection || !mFpAtraceEndSection) ||
            !(mFpAtraceGetEnabledTags() & ATRACE_TAG_NNAPI)) {
            dlclose(mHandle);
            mHandle = nullptr;
        }
    }

    ~ATracerAndroid() {
        if (mHandle) {
            dlclose(mHandle);
        }
    }

    void BeginSection(const char* name) override {
        if (mHandle) {
            mFpAtraceBeginSection(name);
        }
    }

    void EndSection() override {
        if (mHandle) {
            mFpAtraceEndSection();
        }
    }

private:
    using FpGetEnablesTags = std::add_pointer<uint64_t()>::type;
    using FpBeginSection = std::add_pointer<void(const char*)>::type;
    using FpEndSection = std::add_pointer<void()>::type;

    void* mHandle = nullptr;  // Handle to libandroid.so library. Null if not supported.
    FpGetEnablesTags mFpAtraceGetEnabledTags = nullptr;
    FpBeginSection mFpAtraceBeginSection = nullptr;
    FpEndSection mFpAtraceEndSection = nullptr;
};

#else

class ATracerDummy : public ATracerBase {
    void BeginSection(const char* name) override { UNUSED(name); }

    void EndSection() override {}
};

#endif

ATracer* ATracer::Get() {
    static ATracer tracer;
    return &tracer;
}

ATracer::ATracer() {
#if defined(NN_TARGET_NDK)
    mImpl = std::unique_ptr<ATracerBase>(new ATracerNdk);
#elif defined(NN_TARGET_ANDROID)
    mImpl = std::unique_ptr<ATracerBase>(new ATracerAndroid);
#else
    mImpl = std::unique_ptr<ATracerBase>(new ATracerDummy);
#endif
}

}  // namespace npagent
