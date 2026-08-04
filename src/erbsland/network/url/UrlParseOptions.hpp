// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteLength.hpp"

namespace erbsland::network {

/// Limits used while parsing a URL.
/// @tested{UrlTest}
class UrlParseOptions final {
public:
    /// The default maximum source length.
    static constexpr auto cDefaultMaximumLength = unit::ByteLength{16U * 1024U};

public:
    /// Get the maximum source length.
    [[nodiscard]] constexpr auto maximumLength() const noexcept -> unit::ByteLength { return _maximumLength; }
    /// Set the maximum source length.
    constexpr auto setMaximumLength(const unit::ByteLength value) noexcept -> UrlParseOptions & {
        _maximumLength = value;
        return *this;
    }

private:
    unit::ByteLength _maximumLength{cDefaultMaximumLength}; ///< Maximum accepted UTF-8 byte length.
};

}
