// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CombinedBlock.hpp"

#include "../Key.hpp"

#include "../../text/Char.hpp"
#include "../../unit/ByteIndex.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace erbsland::cterm::impl {

/// The status for a decoded terminal key prefix.
enum class KeyParseStatus : uint8_t {
    Invalid,      ///< No valid key prefix was found.
    NeedMoreData, ///< The current bytes are a valid prefix, but need more input.
    Parsed,       ///< A full key was parsed.
};

/// Decode one key from a UTF-8 console input buffer.
class KeyDecoder final {
public:
    /// Result from parsing one UTF-8 code point prefix.
    class CharParseResult final {
    public:
        /// Create an invalid or incomplete character parse result.
        CharParseResult(
            const KeyParseStatus status = KeyParseStatus::Invalid,
            const unit::ByteIndex consumedByteCount = unit::ByteIndex{}) noexcept :
            _status{status}, _consumedByteCount{consumedByteCount} {}

        /// Create a parsed character result.
        CharParseResult(
            const KeyParseStatus status, const text::Char character, const unit::ByteIndex consumedByteCount) noexcept :
            _status{status}, _character{character}, _consumedByteCount{consumedByteCount} {}

    public:
        /// Access the parse status.
        [[nodiscard]] auto status() const noexcept -> KeyParseStatus { return _status; }
        /// Access the parsed character.
        [[nodiscard]] auto character() const noexcept -> text::Char { return _character; }
        /// Access the consumed byte count.
        [[nodiscard]] auto consumedByteCount() const noexcept -> unit::ByteIndex { return _consumedByteCount; }

    private:
        KeyParseStatus _status{KeyParseStatus::Invalid}; ///< The parsing status.
        text::Char _character{};                         ///< The parsed character for `Parsed`.
        unit::ByteIndex _consumedByteCount{};            ///< Number of bytes consumed.
    };

    /// Result of parsing one key from the beginning of a byte stream.
    class ParseResult final {
    public:
        /// Create one parse result without a decoded key.
        /// @param status The parsing status.
        /// @param consumedByteCount The number of bytes consumed.
        ParseResult(
            const KeyParseStatus status = KeyParseStatus::Invalid,
            const unit::ByteIndex consumedByteCount = unit::ByteIndex{}) noexcept :
            _status{status}, _consumedByteCount{consumedByteCount} {}

        /// Create one parse result with a decoded key.
        /// @param status The parsing status.
        /// @param key The decoded key.
        /// @param consumedByteCount The number of bytes consumed.
        ParseResult(const KeyParseStatus status, Key key, const unit::ByteIndex consumedByteCount) noexcept :
            _status{status}, _key{key}, _consumedByteCount{consumedByteCount} {}

    public:
        /// Access the parsing status.
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

public:
    /// Create a decoder for the given input bytes.
    /// @param text The input bytes to decode.
    explicit KeyDecoder(const std::string_view text) noexcept : _text{text} {}

    KeyDecoder(const KeyDecoder &) = delete;
    KeyDecoder(KeyDecoder &&) = delete;
    auto operator=(const KeyDecoder &) -> KeyDecoder & = delete;
    auto operator=(KeyDecoder &&) -> KeyDecoder & = delete;

public:
    /// Parse one key from the beginning of the configured input bytes.
    /// @return The parsing result for the leading bytes.
    [[nodiscard]] auto parseConsoleInputPrefix() const noexcept -> ParseResult;
    /// Decode exactly one full console input item.
    /// @return The decoded key, or an invalid key if the text is unsupported or incomplete.
    [[nodiscard]] auto decodeConsoleInput() const noexcept -> Key;

private:
    struct SimpleSequenceDefinition final {
        std::string_view sequence;
        Key::Type type;
    };

private:
    [[nodiscard]] static auto createCharacterKey(const CombinedBlock &character) noexcept -> Key;
    [[nodiscard]] static auto simpleSequenceDefinitions() noexcept -> const std::array<SimpleSequenceDefinition, 6> &;
    [[nodiscard]] static auto parseModifierParameter(int value) noexcept -> std::optional<KeyModifiers>;
    [[nodiscard]] static auto parseCsiParameters(std::string_view text) noexcept -> std::optional<std::vector<int>>;
    [[nodiscard]] static auto findCsiFinalByte(std::string_view text) noexcept -> std::optional<std::size_t>;
    [[nodiscard]] static auto keyFromCsiFinal(char finalByte) noexcept -> Key::Type;
    [[nodiscard]] static auto keyFromCsiTildeParameter(int parameter) noexcept -> Key::Type;
    [[nodiscard]] static auto decodeCsi(std::string_view text) noexcept -> ParseResult;
    [[nodiscard]] static auto decodeSs3(std::string_view text) noexcept -> ParseResult;
    [[nodiscard]] auto parseUtf8CodePointPrefix(unit::ByteIndex offset) const noexcept -> CharParseResult;

private:
    std::string_view _text;
};

}
