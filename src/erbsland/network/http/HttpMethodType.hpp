// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::network {

/// A recognized standard HTTP method flag.
enum class HttpMethodType : uint16_t {
    None = 0U,             ///< No recognized method.
    Connect = 1U << 0U,    ///< CONNECT.
    Delete = 1U << 1U,     ///< DELETE.
    Get = 1U << 2U,        ///< GET.
    Head = 1U << 3U,       ///< HEAD.
    Options = 1U << 4U,    ///< OPTIONS.
    Patch = 1U << 5U,      ///< PATCH.
    Post = 1U << 6U,       ///< POST.
    Put = 1U << 7U,        ///< PUT.
    Trace = 1U << 8U,      ///< TRACE.
    All = (1U << 9U) - 1U, ///< Every recognized standard method.
};

/// A set of recognized standard HTTP methods.
using HttpMethodTypes = util::EnumFlags<HttpMethodType>;

/// Combine two recognized HTTP method flags.
/// @tested{HttpValueTest}
[[nodiscard]] constexpr auto operator|(const HttpMethodType left, const HttpMethodType right) noexcept
    -> HttpMethodTypes {
    return HttpMethodTypes{left} | right;
}

}
