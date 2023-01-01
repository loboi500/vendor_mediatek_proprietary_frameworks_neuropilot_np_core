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

#pragma once

#include <android-base/expected.h>

#include <string>
#include <tuple>
#include <utility>

#include "HalInterfaces.h"

namespace mediatek {
namespace neuropilotagent {

template <typename Type>
using Result = android::base::expected<Type, std::string>;

struct GeneralError {
    std::string message;
    ErrorStatus code = ErrorStatus::GENERAL_FAILURE;
};

template <typename Type>
using GeneralResult = android::base::expected<Type, GeneralError>;

namespace detail {

template <typename... Ts>
class ErrorBuilder {
public:
    template <typename... Us>
    explicit ErrorBuilder(Us&&... args) : mArgs(std::forward<Us>(args)...) {}

    template <typename T, typename E>
    operator android::base::expected<T, E>() /* NOLINT(google-explicit-constructor) */ {
        return std::apply(
            [this](Ts&&... args) {
                return android::base::unexpected<E>(
                    E{std::move(mStream).str(), std::move(args)...});
            },
            std::move(mArgs));
    }

    template <typename T>
    ErrorBuilder operator<<(const T& t) {
        mStream << t;
        return std::move(*this);
    }

private:
    std::tuple<Ts...> mArgs;
    std::ostringstream mStream;
};

}  // namespace detail

template <typename... Types>
inline detail::ErrorBuilder<std::decay_t<Types>...> error(Types&&... args) {
    return detail::ErrorBuilder<std::decay_t<Types>...>(std::forward<Types>(args)...);
}

#define NN_ERROR(...)                                                     \
    [&] {                                                                 \
        using ::mediatek::neuropilotagent::error;                         \
        return error(__VA_ARGS__) << __FILE__ << ":" << __LINE__ << ": "; \
    }()

template <typename T, typename E>
bool nnTryHasValue(const android::base::expected<T, E>& o) {
    return o.has_value();
}

template <typename T, typename E>
T nnTryGetValue(android::base::expected<T, E> o) {
    return std::move(o).value();
}

template <typename T, typename E>
android::base::unexpected<E> nnTryGetError(android::base::expected<T, E> o) {
    return android::base::unexpected(std::move(o).error());
}

template <typename T>
bool nnTryHasValue(const std::optional<T>& o) {
    return o.has_value();
}

template <typename T>
T nnTryGetValue(std::optional<T> o) {
    return std::move(o).value();
}

template <typename T>
std::nullopt_t nnTryGetError(std::optional<T> /*o*/) {
    return std::nullopt;
}

/**
 * A macro that will exit from the current function if `expr` is unexpected or return the expected
 * value from the macro if `expr` is expected.
 *
 * This macro can currently be used on `::android::nn::Result`, `::android::base::expected`, or
 * `std::optional` values. To enable this macro to be used with other values, implement the
 * following functions for the type:
 * * `::android::nn::nnTryHasValue` returns `true` if the `expr` holds a successful value, false if
 *    the `expr` value holds an error
 * * `::android::nn::nnTryGetError` returns the successful value of `expr` or crashes
 * * `::android::nn::nnTryGetValue` returns the error value of `expr` or crashes
 *
 * Usage at call site:
 *     const auto [a, b, c] = NN_TRY(failableFunction(args));
 */
#define NN_TRY(expr)                                               \
    ({                                                             \
        using ::mediatek::neuropilotagent::nnTryHasValue;          \
        using ::mediatek::neuropilotagent::nnTryGetValue;          \
        using ::mediatek::neuropilotagent::nnTryGetError;          \
        auto nnTryTemporaryResult = expr;                          \
        if (!nnTryHasValue(nnTryTemporaryResult)) {                \
            return nnTryGetError(std::move(nnTryTemporaryResult)); \
        }                                                          \
        nnTryGetValue(std::move(nnTryTemporaryResult));            \
    })

}  // namespace neuropilotagent
}  // namespace mediatek
