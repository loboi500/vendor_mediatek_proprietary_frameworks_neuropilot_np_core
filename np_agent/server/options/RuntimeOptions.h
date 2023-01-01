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

#pragma once

#include <string>

#include <android-base/logging.h>
#include <android-base/macros.h>

namespace mediatek {
namespace neuropilotagent {

// RuntimeOptionsBase is a base type that defines constant values for RuntimeOptions.
class RuntimeOptionsBase {
public:
    RuntimeOptionsBase() = default;

    // System property names
#define DEF_OPTION(Prefix, Id, DefaultValue) \
    static constexpr const char* k##Id##Property = "debug.npagent." Prefix "." #Id;
#include "RuntimeOptions.def"  // NOLINT(build/include)

protected:
    bool IsEnabled(const char* property, bool defaultValue) const;

    int64_t GetInt(const char* property, int64_t defaultValue) const;

    // GetPositiveInt returns a positive value.
    // Otherwise, return the default value 0 (while 0 is not a positive integer, anyway).
    uint64_t GetPositiveInt(const char* property, uint64_t defaultValue) const;

    std::string GetString(const char* property, const char* defaultValue) const;

private:
    DISALLOW_COPY_AND_ASSIGN(RuntimeOptionsBase);
};

// Default implementation of RuntimeOptions
// RuntimeOptions reads system property to enable or disable features at runtime.
class RuntimeOptions : public RuntimeOptionsBase {
public:
    RuntimeOptions() = default;

public:
#define DEF_BOOLEAN_OPTION(Prefix, Id, DefaultValue) \
    bool Id() const { return IsEnabled(k##Id##Property, DefaultValue); }
#define DEF_POSITIVE_INT_OPTION(Prefix, Id, DefaultValue) \
    uint64_t Id() const { return GetPositiveInt(k##Id##Property, DefaultValue); }
#define DEF_STRING_OPTION(Prefix, Id, DefaultValue)                             \
    std::string Id() const { return GetString(k##Id##Property, DefaultValue); } \
    bool Has##Id() const { return (Id().size() != 0); }
#include "RuntimeOptions.def"  // NOLINT(build/include)

private:
    DISALLOW_COPY_AND_ASSIGN(RuntimeOptions);
};

}  // namespace neuropilotagent
}  // namespace mediatek
