// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BaseNFormat_fwd.hpp"
#include "BaseNFormatFlag.hpp"

#include "../Char.hpp"
#include "../CharSet.hpp"
#include "../u32/U32String.hpp"

#include "../../unit/CpCount.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace erbsland::text::base_n {

/// A validated alphabet and behavior definition for power-of-two Base-N encodings.
/// Supported alphabets contain exactly 16, 32, or 64 distinct Unicode scalar values.
/// @seedoc{/reference/text/encoding}
/// @tested{BaseNFormatTest BaseNCodecTest}
class BaseNFormat final {
public:
    /// Create the canonical Base64 format.
    BaseNFormat();
    /// Create an unpadded format for a custom alphabet.
    /// ASCII whitespace is accepted while decoding.
    /// @param alphabet Exactly 16, 32, or 64 distinct Unicode scalar values. Must not contain ASCII whitespace.
    explicit BaseNFormat(U32String alphabet);

    // defaults
    ~BaseNFormat() = default;
    BaseNFormat(const BaseNFormat &) = default;
    BaseNFormat(BaseNFormat &&) noexcept = default;
    auto operator=(const BaseNFormat &) -> BaseNFormat & = default;
    auto operator=(BaseNFormat &&) noexcept -> BaseNFormat & = default;

public: // accessors
    /// Get the digit alphabet.
    [[nodiscard]] auto alphabet() const noexcept -> const U32String & { return _alphabet; }
    /// Set the digit alphabet.
    /// @param alphabet Exactly 16, 32, or 64 distinct Unicode scalar values. Must not overlap the padding or whitespace
    /// characters.
    auto setAlphabet(U32String alphabet) -> BaseNFormat &;
    /// Get the optional padding character.
    [[nodiscard]] auto padding() const noexcept -> std::optional<Char> { return _padding; }
    /// Set or clear the padding character.
    /// @param padding The optional Unicode scalar padding character. Must not overlap the alphabet or whitespace;
    /// required by enabled padding flags.
    auto setPadding(std::optional<Char> padding) -> BaseNFormat &;
    /// Get the characters ignored while decoding.
    [[nodiscard]] auto whitespace() const noexcept -> const CharSet & { return _whitespace; }
    /// Set the characters ignored while decoding.
    /// @param whitespace The ignored characters. Must not overlap the alphabet or padding and must include every
    /// separator character when wrapping is enabled.
    auto setWhitespace(CharSet whitespace) -> BaseNFormat &;
    /// Get the active behavior flags.
    [[nodiscard]] auto flags() const noexcept -> BaseNFormatFlags { return _flags; }
    /// Set the behavior flags.
    /// @param flags The behavior flags. Padding flags require a padding character; wrapping requires a positive finite
    /// line length and a non-empty whitespace-only separator.
    auto setFlags(BaseNFormatFlags flags) -> BaseNFormat &;
    /// Add behavior flags.
    /// @param flags The flags to add. Padding flags require a padding character; wrapping requires a positive finite
    /// line length and a non-empty whitespace-only separator.
    auto addFlags(BaseNFormatFlags flags) -> BaseNFormat &;
    /// Clear behavior flags.
    /// @param flags The flags to clear. The resulting format must retain valid padding and line-wrapping settings.
    auto clearFlags(BaseNFormatFlags flags) -> BaseNFormat &;
    /// Test whether a behavior flag is set.
    [[nodiscard]] auto hasFlag(BaseNFormatFlag flag) const noexcept -> bool { return _flags.contains(flag); }
    /// Get the encoded digit count per line.
    [[nodiscard]] auto lineLength() const noexcept -> unit::CpCount { return _lineLength; }
    /// Set the encoded digit count per line.
    /// @param lineLength The encoded digit count per line. Must be positive and finite when line wrapping is enabled.
    auto setLineLength(unit::CpCount lineLength) -> BaseNFormat &;
    /// Get the separator inserted between encoded lines.
    [[nodiscard]] auto lineSeparator() const noexcept -> const U32String & { return _lineSeparator; }
    /// Set the separator inserted between encoded lines.
    /// @param lineSeparator The separator between lines. When wrapping is enabled, must be non-empty and consist only
    /// of accepted whitespace characters.
    auto setLineSeparator(U32String lineSeparator) -> BaseNFormat &;

public: // encoding tools
    /// Get the number of bits represented by one alphabet character.
    [[nodiscard]] auto bitsPerCharacter() const noexcept -> uint8_t;
    /// Find the value represented by a character.
    [[nodiscard]] auto valueFor(Char character) const noexcept -> std::optional<uint8_t>;
    /// Get the alphabet character for a value.
    [[nodiscard]] auto characterFor(uint8_t value) const noexcept -> Char;

public: // factories
    /// Create the default canonical Base64 format.
    [[nodiscard]] static auto defaultFormat() -> BaseNFormat;
    /// Create the canonical RFC 4648 Base16 format.
    [[nodiscard]] static auto base16() -> BaseNFormat;
    /// Create the canonical RFC 4648 Base32 format.
    [[nodiscard]] static auto base32() -> BaseNFormat;
    /// Create the canonical RFC 4648 extended-hex Base32 format.
    [[nodiscard]] static auto base32Hex() -> BaseNFormat;
    /// Create the canonical RFC 4648 Base64 format.
    [[nodiscard]] static auto base64() -> BaseNFormat;
    /// Create the canonical RFC 4648 URL-safe Base64 format.
    [[nodiscard]] static auto base64Url() -> BaseNFormat;
    /// Create padded Base64 output wrapped at 64 characters with LF separators.
    [[nodiscard]] static auto base64Pem() -> BaseNFormat;

private:
    /// Create a base-N format from normalized encoding options.
    BaseNFormat(
        U32String alphabet,
        std::optional<Char> padding,
        CharSet whitespace,
        BaseNFormatFlags flags,
        unit::CpCount lineLength,
        U32String lineSeparator);
    /// Validate base-N formatting options.
    void validate() const;
    /// Rebuild the ASCII digit lookup table.
    void rebuildAsciiLookup() noexcept;

private:
    U32String _alphabet;
    std::optional<Char> _padding;
    CharSet _whitespace;
    BaseNFormatFlags _flags;
    unit::CpCount _lineLength{64U};
    U32String _lineSeparator;
    std::array<int8_t, 128> _asciiLookup{};
};

}
