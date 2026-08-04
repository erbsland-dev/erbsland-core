// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteFormat_fwd.hpp"
#include "ByteFormatFlag.hpp"
#include "LetterCase.hpp"
#include "String_fwd.hpp"
#include "TruncateMode.hpp"

#include "impl/ByteFormatData_fwd.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ItemCount.hpp"

#include <memory>

namespace erbsland::text {

/// Options for byte block hexadecimal text formatting.
/// @tested{ByteFormatTest}
class ByteFormat final {
public:
    /// Create the default compact byte format.
    ByteFormat();
    /// Create a byte format with the given flags.
    ByteFormat(ByteFormatFlags flags); // NOLINT(*-explicit-constructor)

    /// Destroy this byte format.
    ~ByteFormat();
    /// Copy another byte format.
    ByteFormat(const ByteFormat &);

    // defaults
    ByteFormat(ByteFormat &&) = default;
    /// Copy another byte format into this byte format.
    auto operator=(const ByteFormat &) -> ByteFormat &;

    // defaults
    auto operator=(ByteFormat &&) -> ByteFormat & = default;

public: // accessors
    /// Get the format flags.
    [[nodiscard]] auto flags() const noexcept -> ByteFormatFlags;
    /// Set the format flags.
    auto setFlags(ByteFormatFlags flags) noexcept -> ByteFormat &;
    /// Add format flags.
    auto addFlags(ByteFormatFlags flags) noexcept -> ByteFormat &;
    /// Clear format flags.
    auto clearFlags(ByteFormatFlags flags) noexcept -> ByteFormat &;
    /// Test if the given format flag is set.
    [[nodiscard]] auto hasFlag(ByteFormatFlag flag) const noexcept -> bool;
    /// Get the letter case for hexadecimal digits.
    [[nodiscard]] auto letterCase() const noexcept -> LetterCase;
    /// Set the letter case for hexadecimal digits.
    auto setLetterCase(LetterCase letterCase) noexcept -> ByteFormat &;
    /// Get the number of bytes per line.
    [[nodiscard]] auto bytesPerLine() const noexcept -> unit::ByteLength;
    /// Set the number of bytes per line.
    auto setBytesPerLine(unit::ByteLength bytesPerLine) noexcept -> ByteFormat &;
    /// Get the number of bytes per group.
    [[nodiscard]] auto byteGroupSize() const noexcept -> unit::ByteLength;
    /// Set the number of bytes per group.
    auto setByteGroupSize(unit::ByteLength byteGroupSize) noexcept -> ByteFormat &;
    /// Get the number of lines per group.
    [[nodiscard]] auto lineGroupSize() const noexcept -> unit::ItemCount;
    /// Set the number of lines per group.
    auto setLineGroupSize(unit::ItemCount lineGroupSize) noexcept -> ByteFormat &;
    /// Get the byte or byte group separator.
    [[nodiscard]] auto byteSeparator() const noexcept -> const String &;
    /// Set the byte or byte group separator.
    auto setByteSeparator(const String &byteSeparator) noexcept -> ByteFormat &;
    /// Get the separator between offsets and byte data.
    [[nodiscard]] auto offsetSeparator() const noexcept -> const String &;
    /// Set the separator between offsets and byte data.
    auto setOffsetSeparator(const String &offsetSeparator) -> ByteFormat &;
    /// Get the prefix inserted before each byte-data line.
    [[nodiscard]] auto linePrefix() const noexcept -> const String &;
    /// Set the prefix inserted before each byte-data line.
    auto setLinePrefix(const String &linePrefix) -> ByteFormat &;
    /// Get the suffix inserted after each byte-data line.
    [[nodiscard]] auto lineSuffix() const noexcept -> const String &;
    /// Set the suffix inserted after each byte-data line.
    auto setLineSuffix(const String &lineSuffix) -> ByteFormat &;
    /// Get the starting byte offset.
    [[nodiscard]] auto startOffset() const noexcept -> unit::ByteIndex;
    /// Set the starting byte offset.
    auto setStartOffset(unit::ByteIndex startOffset) noexcept -> ByteFormat &;
    /// Get the maximum number of byte-like output items.
    [[nodiscard]] auto maximum() const noexcept -> unit::ByteLength;
    /// Set the maximum number of byte-like output items.
    auto setMaximum(unit::ByteLength maximum) noexcept -> ByteFormat &;
    /// Get the truncation mode.
    [[nodiscard]] auto truncateMode() const noexcept -> TruncateMode;
    /// Set the truncation mode.
    auto setTruncateMode(TruncateMode truncateMode) noexcept -> ByteFormat &;
    /// Get the ellipsis inserted when bytes are truncated.
    [[nodiscard]] auto ellipsis() const noexcept -> const String &;
    /// Set the ellipsis inserted when bytes are truncated.
    auto setEllipsis(const String &ellipsis) -> ByteFormat &;

public: // factories
    /// Create the default compact format.
    [[nodiscard]] static auto defaultFormat() -> ByteFormat;
    /// Create the default compact format.
    [[nodiscard]] static auto compact() -> ByteFormat;
    /// Create a single-line format with spaces between bytes.
    [[nodiscard]] static auto separated() -> ByteFormat;
    /// Create a multi-line memory dump format with offsets and byte groups.
    [[nodiscard]] static auto memoryDump() -> ByteFormat;
    /// Create a compact diagnostic format limited to sixteen output items.
    [[nodiscard]] static auto forDiagnostic() -> ByteFormat;

private:
    /// Clamp a byte length to at least one.
    [[nodiscard]] static auto atLeastOne(unit::ByteLength value) noexcept -> unit::ByteLength;
    /// Clamp an item count to at least one.
    [[nodiscard]] static auto atLeastOne(unit::ItemCount value) noexcept -> unit::ItemCount;

private:
    std::unique_ptr<impl::ByteFormatData> _p; ///< The private implementation details.
};

}
