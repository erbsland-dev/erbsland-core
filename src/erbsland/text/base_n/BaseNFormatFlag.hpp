// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::text::base_n {

/// Flags controlling Base-N encoding and decoding.
enum class BaseNFormatFlag : uint8_t {
    EmitPadding = 1U << 0U,    ///< Emit canonical padding while encoding.
    RequirePadding = 1U << 1U, ///< Require canonical padding while decoding.
    WrapLines = 1U << 2U,      ///< Wrap encoded output into lines.
    All = (1U << 0U) | (1U << 1U) | (1U << 2U),
};

/// A set of Base-N format flags.
using BaseNFormatFlags = util::EnumFlags<BaseNFormatFlag>;

/// Combine two Base-N format flags.
[[nodiscard]] constexpr auto operator|(BaseNFormatFlag left, BaseNFormatFlag right) noexcept -> BaseNFormatFlags {
    return BaseNFormatFlags{left} | right;
}

}
