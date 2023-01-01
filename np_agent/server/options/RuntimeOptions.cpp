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
#define LOG_TAG "RuntimeOptions"

#include "RuntimeOptions.h"

#include <android-base/logging.h>
#include <android-base/properties.h>

#include "Utils.h"

#include <string>

namespace mediatek {
namespace neuropilotagent {

static const bool kUseDefaultValue = utils::CheckBuildType("user");

#define DEF_POSITIVE_INT_OPTION(Prefix, Id, DefaultValue) \
    static_assert(DefaultValue >= 0,                      \
                  "The default values of positive integer options should not be less than zero");

#define DEF_STRING_OPTION(Prefix, Id, DefaultValue) \
    static_assert(DefaultValue != nullptr,          \
                  "The default values of string options should not be nullptr");

#include "RuntimeOptions.def"  // NOLINT(build/include)

bool RuntimeOptionsBase::IsEnabled(const char* property, bool defaultValue) const {
    if (kUseDefaultValue || property == nullptr) return defaultValue;

    return android::base::GetBoolProperty(property, defaultValue);
}

int64_t RuntimeOptionsBase::GetInt(const char* property, int64_t defaultValue) const {
    if (kUseDefaultValue || property == nullptr) return defaultValue;

    return android::base::GetIntProperty(property, defaultValue);
}

uint64_t RuntimeOptionsBase::GetPositiveInt(const char* property, uint64_t defaultValue) const {
    if (kUseDefaultValue || property == nullptr) return defaultValue;

    return android::base::GetUintProperty(property, defaultValue);
}

std::string RuntimeOptionsBase::GetString(const char* property, const char* defaultValue) const {
    if (kUseDefaultValue || property == nullptr)
        return (defaultValue == nullptr ? "" : defaultValue);

    if (defaultValue == nullptr) return "";

    return android::base::GetProperty(property, defaultValue);
}

}  // namespace neuropilotagent
}  // namespace mediatek
