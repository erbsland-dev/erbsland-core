// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyModifiers.hpp"

#include "impl/CombinedBlock.hpp"
#include "impl/HashHelper.hpp"

#include "../text/Char.hpp"
#include "../text/String.hpp"
#include "../text/StringBuilder_fwd.hpp"
#include "../text/StringLiteral.hpp"
#include "../text/StringView.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u32/U32StringView.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace erbsland::cterm {

/// A simple representation of a key press.
/// Supports Unicode text input and common special keys.
class Key {
public:
    /// Supported key kinds.
    enum Type : uint8_t {
        None,      ///< No supported key was decoded.
        Character, ///< A single Unicode code point.
        Combined,  ///< Multiple code points that form one combined text input.
        Enter,     ///< The Enter/Return key.
        Tab,       ///< The tab key.
        BackTab,   ///< Reverse tab / Shift+Tab.
        Space,     ///< The space key.
        Escape,    ///< The escape key.
        Backspace, ///< The backspace key.
        Insert,    ///< The insert key.
        Delete,    ///< The delete key.
        Home,      ///< The home key.
        End,       ///< The end key.
        PageUp,    ///< The page up key.
        PageDown,  ///< The page down key.
        Left,      ///< The left cursor key.
        Right,     ///< The right cursor key.
        Up,        ///< The up cursor key.
        Down,      ///< The down cursor key.
        F1,        ///< The function key F1.
        F2,        ///< The function key F2.
        F3,        ///< The function key F3.
        F4,        ///< The function key F4.
        F5,        ///< The function key F5.
        F6,        ///< The function key F6.
        F7,        ///< The function key F7.
        F8,        ///< The function key F8.
        F9,        ///< The function key F9.
        F10,       ///< The function key F10.
        F11,       ///< The function key F11.
        F12,       ///< The function key F12.
    };

public:
    /// Create an invalid key.
    Key() = default;
    /// Create a key with an explicit type and optional Unicode payload.
    /// @param type The key type.
    /// @param codePoint The Unicode value for `Type::Character`.
    /// @param modifiers The modifiers pressed together with this key.
    Key(Type type, text::Char codePoint = {}, KeyModifiers modifiers = {}) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a special key with modifiers.
    /// @param type The key type.
    /// @param modifiers The modifiers pressed together with this key.
    Key(Type type, KeyModifiers modifiers) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a single-code-point character key.
    /// @param codePoint The Unicode code point.
    /// @param modifiers The modifiers pressed together with this key.
    Key(text::Char codePoint, KeyModifiers modifiers = {}) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a key with an explicit combined Unicode payload.
    /// @param type The key type.
    /// @param character The combined Unicode text for `Type::Character` or `Type::Combined`.
    /// @param modifiers The modifiers pressed together with this key.
    /// @throws std::invalid_argument If `character` is not a supported Unicode character sequence.
    Key(Type type, const text::U32StringView &character, KeyModifiers modifiers = {});

    // defaults
    ~Key() = default;
    Key(const Key &) = default;
    Key(Key &&) = default;
    auto operator=(const Key &) -> Key & = default;
    auto operator=(Key &&) -> Key & = default;

public: // operators
    /// Compare two key events for equality.
    auto operator==(const Key &other) const noexcept -> bool = default;
    auto operator!=(const Key &other) const noexcept -> bool = default;
    /// Compare against a single code point.
    /// This requires `type()` == `Character` and `unicode()` == `other`.
    [[nodiscard]] auto operator==(text::Char other) const noexcept -> bool;
    [[nodiscard]] auto operator!=(text::Char other) const noexcept -> bool;
    /// Compare against a combined key
    /// This requires `type()` == `Combined` and `combined()` == `other`.
    [[nodiscard]] auto operator==(const text::U32StringView &other) const noexcept -> bool;
    [[nodiscard]] auto operator!=(const text::U32StringView &other) const noexcept -> bool;
    /// Compare against a special key.
    /// This requires `type()` == `type` and `type` != `Character`|`Combined`.
    [[nodiscard]] auto operator==(Type type) const noexcept -> bool;
    [[nodiscard]] auto operator!=(Type type) const noexcept -> bool;

public: // accessors
    /// Get the key type.
    [[nodiscard]] auto type() const noexcept -> Type { return _type; }
    /// Get the modifiers pressed together with this key.
    [[nodiscard]] auto modifiers() const noexcept -> const KeyModifiers & { return _modifiers; }
    /// Test if a modifier is set.
    [[nodiscard]] auto hasModifier(KeyModifier modifier) const noexcept -> bool { return _modifiers.has(modifier); }
    /// Create a copy of this key without modifiers.
    [[nodiscard]] auto withoutModifiers() const noexcept -> Key;
    /// Legacy ASCII accessor for `Type::Character`.
    /// @deprecated Use `unicode()` or `combined()` to support full Unicode input.
    /// @return The ASCII character for single-code-point character input, otherwise `0`.
    [[nodiscard]] auto character() const noexcept -> char;
    /// Get the Unicode code point for `Type::Character`.
    /// @return The single Unicode code point, or `0` if this key does not store exactly one code point.
    [[nodiscard]] auto unicode() const noexcept -> text::Char;
    /// Get the full combined Unicode payload for character input.
    /// @return The stored Unicode text, or an empty string for non-character keys.
    [[nodiscard]] auto combined() const -> text::U32String;
    /// Test if this object represents a supported key.
    [[nodiscard]] auto valid() const noexcept -> bool { return _type != None; }
    /// Get a hash for this key.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return impl::hashCreate(static_cast<uint8_t>(_type), _character.hash(), _modifiers.mask());
    }

public: // conversion
    /// Decode a key from the configuration text.
    /// @param text The textual key name.
    /// @return The decoded key, or `Type::None` if the text is unsupported.
    [[nodiscard]] static auto fromString(const text::StringView &text) noexcept -> Key;
    /// Decode a key from console input text.
    /// @param text The input text or escape sequence.
    /// @return The decoded key, or `Type::None` if the input is unsupported.
    [[nodiscard]] static auto fromConsoleInput(const text::StringView &text) noexcept -> Key;
    /// Convert the key to configuration text.
    /// @return The canonical textual key name.
    [[nodiscard]] auto toString() const -> text::String;
    /// Convert the key to human-readable display text.
    /// @param useBrackets If `true`, wrap the text in `[` and `]`.
    /// @return The display text for prompts and help texts.
    [[nodiscard]] auto toDisplayText(bool useBrackets = true) const -> text::String;

private:
    struct KeyTextDefinition final {
        Type type;
        text::StringLiteral text;
        text::StringLiteral displayText;
    };

    struct KeyAliasDefinition final {
        text::StringLiteral text;
        Type type;
    };

    /// Get the canonical text definitions for special keys.
    [[nodiscard]] static auto keyTextDefinitions() noexcept -> const std::array<KeyTextDefinition, 28> &;
    /// Get the accepted alias definitions for parsing key names.
    [[nodiscard]] static auto keyAliasDefinitions() noexcept -> const std::array<KeyAliasDefinition, 39> &;
    /// Find the display and serialization text for a special key.
    [[nodiscard]] static auto findKeyTextDefinition(Type type) noexcept -> std::optional<KeyTextDefinition>;
    /// Normalize a key name for case-insensitive alias matching.
    [[nodiscard]] static auto normalizeKeyText(const text::StringView &text) -> text::String;
    /// Remove parsed modifiers from the beginning of a key string.
    [[nodiscard]] static auto parseModifiers(text::String &text) noexcept -> KeyModifiers;
    /// Parse one modifier name.
    [[nodiscard]] static auto parseModifierText(const text::StringView &text) noexcept -> std::optional<KeyModifier>;
    /// Append modifier configuration text to a string.
    static void appendModifierString(text::StringBuilder &builder, KeyModifiers modifiers);
    /// Append modifier display text to a string.
    static void appendModifierDisplayText(text::StringBuilder &builder, KeyModifiers modifiers);
    /// Wrap display text in square brackets when requested.
    [[nodiscard]] static auto wrapDisplayText(const text::StringView &text, bool useBrackets) -> text::String;
    /// Create a character or combined key from normalized Unicode input.
    [[nodiscard]] static auto createCharacterKey(const impl::CombinedBlock &character) noexcept -> Key;
    /// Parse Unicode text into a character key when possible.
    [[nodiscard]] static auto parseCharacterKeyText(const text::StringView &text) -> std::optional<Key>;

private:
    Type _type{None};
    impl::CombinedBlock _character;
    KeyModifiers _modifiers;
};

}

template <>
struct std::hash<erbsland::cterm::Key> {
    auto operator()(const erbsland::cterm::Key &key) const noexcept -> std::size_t { return key.hash(); }
};
