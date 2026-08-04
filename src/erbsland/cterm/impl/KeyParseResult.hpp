// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyParseStatus.hpp"

#include "../Key.hpp"

#include "../../unit/ByteIndex.hpp"

#include <utility>

namespace erbsland::cterm::impl {

/// Store the outcome of parsing one key from a byte stream.
class KeyParseResult final {
public:
    /// Create one parse result without a decoded key.
    /// @param status The parsing status.
    /// @param consumedByteCount The number of bytes consumed.
    KeyParseResult(
        const KeyParseStatus status = KeyParseStatus::Invalid,
        const unit::ByteIndex consumedByteCount = unit::ByteIndex{}) noexcept :
        _status{status}, _consumedByteCount{consumedByteCount} {}

    /// Create one parse result with a decoded key.
    /// @param status The parsing status.
    /// @param key The decoded key.
    /// @param consumedByteCount The number of bytes consumed.
    KeyParseResult(const KeyParseStatus status, Key key, const unit::ByteIndex consumedByteCount) noexcept :
        _status{status}, _key{std::move(key)}, _consumedByteCount{consumedByteCount} {}

    /// Access the parse status.
    [[nodiscard]] auto status() const noexcept -> KeyParseStatus { return _status; }
    /// Access the parsed key for `Parsed`.
    [[nodiscard]] auto key() const noexcept -> const Key & { return _key; }
    /// Access the number of bytes consumed.
    [[nodiscard]] auto consumedByteCount() const noexcept -> unit::ByteIndex { return _consumedByteCount; }

private:
    KeyParseStatus _status{KeyParseStatus::Invalid}; ///< The parsing status.
    Key _key;                                        ///< The parsed key for `Parsed`.
    unit::ByteIndex _consumedByteCount{};            ///< Number of bytes consumed.
};

}
