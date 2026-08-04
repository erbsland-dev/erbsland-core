// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteBlock.hpp"
#include "../ByteIntegerFormat.hpp"
#include "../ByteTextOptions_fwd.hpp"
#include "../ByteWriter_fwd.hpp"

#include "../../text/String_fwd.hpp"

#include <cstdint>

namespace erbsland::mem::impl {

/// Implements structured writes for `ByteWriter`.
/// @tested{ByteReaderWriterTest}
class ByteWriterTools final {
public:
    /// Create tools for the given sequential writer.
    explicit ByteWriterTools(ByteWriter &writer) noexcept : _writer{writer} {}

public:
    /// Write an integer sign and magnitude using an explicit wire format.
    /// @param isNegative Whether the value is negative.
    /// @param magnitude The unsigned absolute value.
    /// @param format The wire format.
    /// @return The bound writer.
    /// @throws err::OutOfRangeError If the value cannot be represented by `format`.
    void writeIntegerOrThrow(bool isNegative, uint64_t magnitude, ByteIntegerFormat format);
    /// Encode and write a complete structured text frame.
    /// @param text The text to encode.
    /// @param options The text framing options.
    /// @param strict Whether text that exceeds a configured limit causes an error instead of truncation.
    /// @return The bound writer.
    /// @throws err::OutOfRangeError If the framing is invalid or cannot contain the requested text.
    void writeText(const text::String &text, const ByteTextOptions &options, bool strict);

private:
    /// Convert a signed magnitude to the raw representation required by a format.
    [[nodiscard]] auto rawValueOrThrow(bool isNegative, uint64_t magnitude, ByteIntegerFormat format) const -> uint64_t;
    /// Encode a fixed-width integer into the writer.
    void encodeStaticInteger(uint64_t value, unit::ByteLength byteCount) const;
    /// Encode a variable-width integer into the writer.
    void encodeVariableInteger(uint64_t value) const;
    /// Encode a base-128 variable-width integer into the writer.
    void encodeBase128Integer(uint64_t value) const;
    /// Build a framed text block before committing it to the writer.
    [[nodiscard]] auto makeTextFrame(const text::String &text, const ByteTextOptions &options, bool strict)
        -> ByteBlock;

private:
    ByteWriter &_writer; ///< The sequential writer receiving encoded values.
};

}
