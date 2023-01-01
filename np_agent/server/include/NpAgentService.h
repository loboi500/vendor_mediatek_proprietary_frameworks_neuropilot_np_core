/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#include <android/log.h>
#include <dlfcn.h>

#define NPAGENT_LOG_D(format, ...) \
    __android_log_print(ANDROID_LOG_DEBUG, "NpAgentServer", format "\n", ##__VA_ARGS__);

#define NPAGENT_LOG_E(format, ...) \
    __android_log_print(ANDROID_LOG_ERROR, "NpAgentServer", format "\n", ##__VA_ARGS__);

#define LOAD_NPAGENT_FUNCTION(name) \
    static name##_fn fnNpAgent = reinterpret_cast<name##_fn>(loadNpAgentFunction(#name));

#define EXECUTE_NPAGENT_FUNCTION(...) \
    if (fnNpAgent != NULL) {          \
        fnNpAgent(__VA_ARGS__);       \
    }

static void* sNpAgentLibHandle;

inline void* loadNpAgentLibrary(const char* name) {
    sNpAgentLibHandle = dlopen(name, RTLD_LAZY | RTLD_LOCAL);

    if (sNpAgentLibHandle == nullptr) {
        char* error = nullptr;
        if ((error = dlerror()) != nullptr) {
            NPAGENT_LOG_E("unable to open NpAgent library %s, with error %s", name, error);
        }
        return nullptr;
    } else {
        NPAGENT_LOG_D("open NpAgent library %s", name);
    }
    return sNpAgentLibHandle;
}

inline void* getNpAgentLibraryHandle() {
    if (sNpAgentLibHandle == nullptr) {
        sNpAgentLibHandle = loadNpAgentLibrary("libnpagent_server.so");
    }
    return sNpAgentLibHandle;
}

inline void* loadNpAgentFunction(const char* name) {
    void* fn = nullptr;
    char* error = nullptr;
    if (getNpAgentLibraryHandle() != nullptr) {
        fn = dlsym(getNpAgentLibraryHandle(), name);
        if (fn == nullptr) {
            if ((error = dlerror()) != nullptr) {
                NPAGENT_LOG_E("unable to open NpAgent function %s, with error %s", name, error);
                return nullptr;
            }
        }
    }
    return fn;
}

typedef void (*NpAgentServerInit_fn)();
typedef void (*NpAgentServerDeinit_fn)();

#ifdef __cplusplus
extern "C" {
#endif

inline void NpAgentServerInit() {
    LOAD_NPAGENT_FUNCTION(NpAgentServerInit);
    EXECUTE_NPAGENT_FUNCTION();
}

inline void NpAgentServerDeinit() {
    LOAD_NPAGENT_FUNCTION(NpAgentServerDeinit);
    EXECUTE_NPAGENT_FUNCTION();
}

#ifdef __cplusplus
}  // extern "C"
#endif
