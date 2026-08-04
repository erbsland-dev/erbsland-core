// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyCodePointParseResult.hpp"
#include "KeyParseResult.hpp"

#include "../../text/CombinedChar.hpp"
#include "../../text/String.hpp"
#include "../../unit/ByteIndex.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace erbsland::cterm::impl {

/// Decode one key from a UTF-8 console input buffer.
class KeyDecoder final {
public:
    /// Create a decoder for the given input bytes.
    /// @param text The input bytes to decode.
    /// @param sensitive Securely erase the copied input bytes when the decoder is destroyed.
    explicit KeyDecoder(text::String text, bool sensitive = false) noexcept : _text{std::move(text)} {
        if (sensitive) {
            _text.markAsSensitive();
        }
    }

    // defaults/deletions
    ~KeyDecoder() = default;
    KeyDecoder(const KeyDecoder &) = delete;
    KeyDecoder(KeyDecoder &&) = delete;
    auto operator=(const KeyDecoder &) -> KeyDecoder & = delete;
    auto operator=(KeyDecoder &&) -> KeyDecoder & = delete;

public:
    /// Parse one key from the beginning of the configured input bytes.
    /// @return The parsing result for the leading bytes.
    [[nodiscard]] auto parseConsoleInputPrefix() const noexcept -> KeyParseResult;
    /// Decode exactly one full console input item.
    /// @return The decoded key, or an invalid key if the text is unsupported or incomplete.
    [[nodiscard]] auto decodeConsoleInput() const noexcept -> Key;

private:
    /// Defines one fixed terminal escape sequence and its key type.
    struct SimpleSequenceDefinition final {
        text::String sequence;
        Key::Type type;
    };

private:
    /// Create a key for a decoded combined character.
    [[nodiscard]] static auto createCharacterKey(const text::CombinedChar &character) noexcept -> Key;
    /// Get definitions for fixed terminal escape sequences.
    [[nodiscard]] static auto simpleSequenceDefinitions() noexcept -> const std::array<SimpleSequenceDefinition, 6> &;
    /// Convert a CSI modifier parameter into key modifiers.
    [[nodiscard]] static auto parseModifierParameter(int value) noexcept -> std::optional<KeyModifiers>;
    /// Parse the numeric parameters of a CSI sequence.
    [[nodiscard]] static auto parseCsiParameters(const text::String &text) noexcept -> std::optional<std::vector<int>>;
    /// Find the final byte of a CSI sequence.
    [[nodiscard]] static auto findCsiFinalByte(const text::String &text) noexcept -> std::optional<unit::ByteIndex>;
    /// Convert a CSI final byte into a key type.
    [[nodiscard]] static auto keyFromCsiFinal(text::Char finalByte) noexcept -> Key::Type;
    /// Convert a CSI tilde parameter into a key type.
    [[nodiscard]] static auto keyFromCsiTildeParameter(int parameter) noexcept -> Key::Type;
    /// Decode one control-sequence-introducer key sequence.
    [[nodiscard]] static auto decodeCsi(const text::String &text) noexcept -> KeyParseResult;
    /// Decode one SS3 key sequence.
    [[nodiscard]] static auto decodeSs3(const text::String &text) noexcept -> KeyParseResult;
    /// Decode one UTF-8 code-point prefix at an input offset.
    [[nodiscard]] static auto decodeCodePointPrefix(const text::String &text, unit::ByteIndex offset) noexcept
        -> KeyCodePointParseResult;

private:
    text::String _text;
};

}
