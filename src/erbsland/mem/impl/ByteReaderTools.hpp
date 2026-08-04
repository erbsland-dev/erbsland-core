// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteReaderTools_fwd.hpp"

#include "../ByteBlock_fwd.hpp"
#include "../ByteIntegerFormat.hpp"
#include "../ByteReader_fwd.hpp"
#include "../ByteTextOptions_fwd.hpp"

#include "../../text/String_fwd.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"

#include <cstdint>

namespace erbsland::mem::impl {

/// Implements structured reads for `ByteReader`.
/// @tested{ByteReaderWriterTest}
class ByteReaderTools final {
public:
    /// Create a new byte reader tools instance.
    explicit ByteReaderTools(ByteReader &reader) noexcept : _reader{reader} {}

public:
    /// Transactionally read structured text.
    /// @param options The text framing options.
    /// @return The decoded text.
    /// @throws err::OutOfRangeError If the complete frame is unavailable.
    /// @throws err::ParseError If the frame is invalid.
    [[nodiscard]] auto readTextOrThrow(const ByteTextOptions &options) -> text::String;

private:
    /// Decode text from a field with a known byte length.
    [[nodiscard]] auto readTextFromFieldOrThrow(
        const ByteTextOptions &options, unit::ByteLength fieldLength, unit::ByteIndex &cursor) -> text::String;
    /// Decode text whose byte length is encoded in the stream.
    [[nodiscard]] auto readDynamicTextOrThrow(const ByteTextOptions &options, unit::ByteIndex &cursor) -> text::String;
    /// Decode an integer count and advance the temporary cursor.
    [[nodiscard]] auto readCountOrThrow(ByteIntegerFormat format, unit::ByteIndex &cursor) const -> uint64_t;
    /// Copy a byte range from the temporary cursor.
    [[nodiscard]] auto readBytesOrThrow(unit::ByteLength length, unit::ByteIndex &cursor) const -> ByteBlock;
    /// Test whether a byte range is available from the temporary cursor.
    [[nodiscard]] auto canRead(unit::ByteIndex cursor, unit::ByteLength length) const noexcept -> bool;

private:
    ByteReader &_reader; ///< The sequential reader whose position is committed after successful parsing.
};

}
