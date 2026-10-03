// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteBlock.hpp"
#include "../ByteTextOptions_fwd.hpp"

#include "../../unit/ByteCount.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::mem::impl {

/// Implements shared calculations for structured byte text.
/// @tested{ByteReaderWriterTest}
class ByteTextTools final {
public:
    /// Create text tools for the given framing options.
    explicit ByteTextTools(const ByteTextOptions &options) noexcept : _options{options} {}

public:
    /// Get the byte width of one code unit in an encoding.
    [[nodiscard]] auto unitSize() const noexcept -> std::size_t;
    /// Encode the configured optional end mark in the selected encoding.
    [[nodiscard]] auto encodedEndMark() const -> ByteBlock;
    /// Convert a code-unit count into a byte length.
    /// @param count The code-unit count from the encoded frame.
    /// @throws err::OverflowError If the encoded count cannot be represented as a byte length.
    [[nodiscard]] auto byteLengthFromCountOrThrow(uint64_t count) const -> unit::ByteCount;

private:
    const ByteTextOptions &_options; ///< The borrowed text framing options.
};

}
