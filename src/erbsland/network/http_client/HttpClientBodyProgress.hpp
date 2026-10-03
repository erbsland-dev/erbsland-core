// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteCount.hpp"

#include <optional>

namespace erbsland::network {

/// Successfully committed response-body progress for an output sink.
/// @tested{HttpClientTest}
class HttpClientBodyProgress final {
public:
    /// Create one progress value.
    HttpClientBodyProgress(unit::ByteCount committedLength, std::optional<unit::ByteCount> expectedLength) noexcept :
        _committedLength{committedLength}, _expectedLength{expectedLength} {}

    /// Get the number of bytes successfully written to the sink.
    [[nodiscard]] constexpr auto committedLength() const noexcept -> unit::ByteCount { return _committedLength; }
    /// Get the expected decoded body length when known from Content-Length.
    [[nodiscard]] constexpr auto expectedLength() const noexcept -> std::optional<unit::ByteCount> {
        return _expectedLength;
    }

private:
    unit::ByteCount _committedLength;               ///< Successfully written bytes.
    std::optional<unit::ByteCount> _expectedLength; ///< Known decoded body length.
};

}
