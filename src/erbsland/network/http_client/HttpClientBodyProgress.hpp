// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteLength.hpp"

#include <optional>

namespace erbsland::network {

/// Successfully committed response-body progress for an output sink.
/// @tested{HttpClientTest}
class HttpClientBodyProgress final {
public:
    /// Create one progress value.
    HttpClientBodyProgress(unit::ByteLength committedLength, std::optional<unit::ByteLength> expectedLength) noexcept :
        _committedLength{committedLength}, _expectedLength{expectedLength} {}

    /// Get the number of bytes successfully written to the sink.
    [[nodiscard]] constexpr auto committedLength() const noexcept -> unit::ByteLength { return _committedLength; }
    /// Get the expected decoded body length when known from Content-Length.
    [[nodiscard]] constexpr auto expectedLength() const noexcept -> std::optional<unit::ByteLength> {
        return _expectedLength;
    }

private:
    unit::ByteLength _committedLength;               ///< Successfully written bytes.
    std::optional<unit::ByteLength> _expectedLength; ///< Known decoded body length.
};

}
