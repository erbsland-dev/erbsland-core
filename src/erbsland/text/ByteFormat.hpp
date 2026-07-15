// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteFormatFlag.hpp"
#include "LetterCase.hpp"
#include "StringView_fwd.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ElementCount.hpp"

#include <memory>

namespace erbsland::text {

/// Options for byte block hexadecimal text formatting.
/// @tested{ByteFormatTest}
class ByteFormat final {
    struct Private;

public:
    /// Create the default compact byte format.
    ByteFormat();
    /// Create a byte format with the given flags.
    ByteFormat(ByteFormatFlags flags); // NOLINT(*-explicit-constructor)

    // defaults
    ~ByteFormat();
    /// Copy this byte format.
    ByteFormat(const ByteFormat &);
    ByteFormat(ByteFormat &&) = default;
    auto operator=(const ByteFormat &) -> ByteFormat &;
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
    [[nodiscard]] auto lineGroupSize() const noexcept -> unit::ElementCount;
    /// Set the number of lines per group.
    auto setLineGroupSize(unit::ElementCount lineGroupSize) noexcept -> ByteFormat &;
    /// Get the byte or byte group separator.
    [[nodiscard]] auto byteSeparator() const noexcept -> const StringView &;
    /// Set the byte or byte group separator.
    auto setByteSeparator(const StringView &byteSeparator) noexcept -> ByteFormat &;
    /// Get the separator between offsets and byte data.
    [[nodiscard]] auto offsetSeparator() const noexcept -> const StringView &;
    /// Set the separator between offsets and byte data.
    auto setOffsetSeparator(const StringView &offsetSeparator) -> ByteFormat &;
    /// Get the prefix inserted before each byte-data line.
    [[nodiscard]] auto linePrefix() const noexcept -> const StringView &;
    /// Set the prefix inserted before each byte-data line.
    auto setLinePrefix(const StringView &linePrefix) -> ByteFormat &;
    /// Get the suffix inserted after each byte-data line.
    [[nodiscard]] auto lineSuffix() const noexcept -> const StringView &;
    /// Set the suffix inserted after each byte-data line.
    auto setLineSuffix(const StringView &lineSuffix) -> ByteFormat &;
    /// Get the starting byte offset.
    [[nodiscard]] auto startOffset() const noexcept -> unit::ByteIndex;
    /// Set the starting byte offset.
    auto setStartOffset(unit::ByteIndex startOffset) noexcept -> ByteFormat &;

public: // factories
    /// Create the default compact format.
    [[nodiscard]] static auto defaultFormat() -> ByteFormat;
    /// Create the default compact format.
    [[nodiscard]] static auto compact() -> ByteFormat;
    /// Create a single-line format with spaces between bytes.
    [[nodiscard]] static auto separated() -> ByteFormat;
    /// Create a multi-line memory dump format with offsets and byte groups.
    [[nodiscard]] static auto memoryDump() -> ByteFormat;

private:
    [[nodiscard]] static auto atLeastOne(unit::ByteLength value) noexcept -> unit::ByteLength;
    [[nodiscard]] static auto atLeastOne(unit::ElementCount value) noexcept -> unit::ElementCount;

private:
    std::unique_ptr<Private> _p; ///< The private implementation details.
};

}
