// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Key.hpp"

#include "impl/KeyDecoder.hpp"

#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"
#include "../unit/CpLength.hpp"

#include <array>
#include <optional>

namespace erbsland::cterm {

using namespace erbsland::text::literals;
using namespace text;

auto Key::keyTextDefinitions() noexcept -> const std::array<KeyTextDefinition, 28> & {
    static constexpr auto cKeyTextDefinitions = std::array<KeyTextDefinition, 28>{{
        {Enter, "enter"_el, "↵"_el},
        {Tab, "tab"_el, "tab"_el},
        {BackTab, "backtab"_el, "⇤"_el},
        {Space, "space"_el, "space"_el},
        {Escape, "escape"_el, "esc"_el},
        {Backspace, "backspace"_el, "⌫"_el},
        {Insert, "insert"_el, "ins"_el},
        {Delete, "delete"_el, "del"_el},
        {Home, "home"_el, "home"_el},
        {End, "end"_el, "end"_el},
        {PageUp, "pageup"_el, "pgup"_el},
        {PageDown, "pagedown"_el, "pgdn"_el},
        {Left, "left"_el, "←"_el},
        {Right, "right"_el, "→"_el},
        {Up, "up"_el, "↑"_el},
        {Down, "down"_el, "↓"_el},
        {F1, "f1"_el, "F1"_el},
        {F2, "f2"_el, "F2"_el},
        {F3, "f3"_el, "F3"_el},
        {F4, "f4"_el, "F4"_el},
        {F5, "f5"_el, "F5"_el},
        {F6, "f6"_el, "F6"_el},
        {F7, "f7"_el, "F7"_el},
        {F8, "f8"_el, "F8"_el},
        {F9, "f9"_el, "F9"_el},
        {F10, "f10"_el, "F10"_el},
        {F11, "f11"_el, "F11"_el},
        {F12, "f12"_el, "F12"_el},
    }};
    return cKeyTextDefinitions;
}

auto Key::keyAliasDefinitions() noexcept -> const std::array<KeyAliasDefinition, 39> & {
    static constexpr auto cKeyAliasDefinitions = std::array<KeyAliasDefinition, 39>{{
        {"enter"_el, Enter},
        {"return"_el, Enter},
        {"tab"_el, Tab},
        {"backtab"_el, BackTab},
        {"back_tab"_el, BackTab},
        {"shift_tab"_el, BackTab},
        {"space"_el, Space},
        {"escape"_el, Escape},
        {"esc"_el, Escape},
        {"backspace"_el, Backspace},
        {"insert"_el, Insert},
        {"ins"_el, Insert},
        {"delete"_el, Delete},
        {"del"_el, Delete},
        {"home"_el, Home},
        {"end"_el, End},
        {"pageup"_el, PageUp},
        {"page_up"_el, PageUp},
        {"pgup"_el, PageUp},
        {"pagedown"_el, PageDown},
        {"page_down"_el, PageDown},
        {"pgdown"_el, PageDown},
        {"pgdn"_el, PageDown},
        {"left"_el, Left},
        {"right"_el, Right},
        {"up"_el, Up},
        {"down"_el, Down},
        {"f1"_el, F1},
        {"f2"_el, F2},
        {"f3"_el, F3},
        {"f4"_el, F4},
        {"f5"_el, F5},
        {"f6"_el, F6},
        {"f7"_el, F7},
        {"f8"_el, F8},
        {"f9"_el, F9},
        {"f10"_el, F10},
        {"f11"_el, F11},
        {"f12"_el, F12},
    }};
    return cKeyAliasDefinitions;
}

auto Key::findKeyTextDefinition(const Type type) noexcept -> std::optional<KeyTextDefinition> {
    for (const auto &definition : keyTextDefinitions()) {
        if (definition.type == type) {
            return definition;
        }
    }
    return std::nullopt;
}

auto Key::normalizeKeyText(const String &text) -> String {
    return text.transformed(Char::toLowercase);
}

auto Key::parseModifierText(const String &text) noexcept -> std::optional<KeyModifier> {
    if (text == "shift"_el) {
        return KeyModifier::Shift;
    }
    if (text == "ctrl"_el || text == "control"_el) {
        return KeyModifier::Control;
    }
    if (text == "alt"_el) {
        return KeyModifier::Alt;
    }
    return std::nullopt;
}

auto Key::parseModifiers(String &text) noexcept -> KeyModifiers {
    static const auto separatorCharacters = CharSet{U'+'};
    auto modifiers = KeyModifiers{};
    while (true) {
        const auto separator = text.findFirstOf(separatorCharacters);
        if (separator.isNoIndex()) {
            return modifiers;
        }
        const auto modifierText = normalizeKeyText(text.slice(unit::ByteRange{unit::ByteIndex::zero(), separator}));
        const auto modifier = parseModifierText(modifierText);
        if (!modifier.has_value()) {
            return modifiers;
        }
        modifiers.set(*modifier);
        text = text.slice(unit::ByteRange{separator + unit::ByteLength::one(), unit::ByteLength::infinite()});
    }
}

void Key::appendModifierString(StringEditor &builder, const KeyModifiers modifiers) {
    if (modifiers.has(KeyModifier::Shift)) {
        builder.append("shift+"_el);
    }
    if (modifiers.has(KeyModifier::Control)) {
        builder.append("ctrl+"_el);
    }
    if (modifiers.has(KeyModifier::Alt)) {
        builder.append("alt+"_el);
    }
}

void Key::appendModifierDisplayText(StringEditor &builder, const KeyModifiers modifiers) {
    if (modifiers.has(KeyModifier::Shift)) {
        builder.append("Shift+"_el);
    }
    if (modifiers.has(KeyModifier::Control)) {
        builder.append("Ctrl+"_el);
    }
    if (modifiers.has(KeyModifier::Alt)) {
        builder.append("Alt+"_el);
    }
}

auto Key::wrapDisplayText(const String &text, const bool useBrackets) -> String {
    if (!useBrackets) {
        return text;
    }
    return String::fromJoined({"["_el, text, "]"_el});
}

auto Key::createCharacterKey(const CombinedChar &character) noexcept -> Key {
    if (character.characterCount() <= unit::CpLength::one()) {
        return Key{Character, character.first()};
    }
    return Key{Combined, character.toU32String()};
}

auto Key::parseCharacterKeyText(const String &text) -> std::optional<Key> {
    if (text.isEmpty()) {
        return std::nullopt;
    }
    return createCharacterKey(CombinedChar::fromString(text));
}

Key::Key(const Type type, const Char codePoint, const KeyModifiers modifiers) noexcept :
    _type{type}, _modifiers{modifiers} {
    if (type == Character || type == Combined) {
        _character = CombinedChar{codePoint};
    }
}

Key::Key(const Type type, const KeyModifiers modifiers) noexcept : Key{type, Char{}, modifiers} {
}

Key::Key(const Char codePoint, const KeyModifiers modifiers) noexcept : Key{Character, codePoint, modifiers} {
}

Key::Key(const Type type, const U32String &character, const KeyModifiers modifiers) :
    _type{type}, _modifiers{modifiers} {
    if (type == Character || type == Combined) {
        _character = CombinedChar{character};
    }
}

auto Key::operator==(const Char other) const noexcept -> bool {
    return _type == Character && _modifiers.empty() && _character.first() == other;
}

auto Key::operator!=(const Char other) const noexcept -> bool {
    return !operator==(other);
}

auto Key::operator==(const U32String &other) const noexcept -> bool {
    return _type == Combined && _modifiers.empty() && combined() == other;
}

auto Key::operator!=(const U32String &other) const noexcept -> bool {
    return !operator==(other);
}

auto Key::operator==(const Type type) const noexcept -> bool {
    if (type == Character || type == Combined) {
        return false;
    }
    return _type == type && _modifiers.empty();
}

auto Key::operator!=(const Type type) const noexcept -> bool {
    return !operator==(type);
}

auto Key::character() const noexcept -> char {
    const auto codePoint = unicode();
    if (codePoint > U'\x7f') {
        return 0;
    }
    return static_cast<char>(codePoint.toRawValue());
}

auto Key::unicode() const noexcept -> Char {
    if (_type != Character || _character.characterCount() != unit::CpLength::one()) {
        return {};
    }
    return _character.first();
}

auto Key::combined() const -> U32String {
    if (_type != Character && _type != Combined) {
        return {};
    }
    return _character.toU32String();
}

auto Key::withoutModifiers() const noexcept -> Key {
    auto result = *this;
    result._modifiers = {};
    return result;
}

auto Key::fromString(const String &text) noexcept -> Key {
    auto parsedText = text;
    const auto modifiers = parseModifiers(parsedText);
    const auto keyText = normalizeKeyText(parsedText);
    for (const auto &definition : keyAliasDefinitions()) {
        if (keyText == definition.text) {
            return Key{definition.type, modifiers};
        }
    }
    if (!modifiers.empty()) {
        return Key{None};
    }
    if (const auto key = parseCharacterKeyText(parsedText); key.has_value()) {
        return *key;
    }
    return Key{None};
}

auto Key::fromConsoleInput(const String &text) noexcept -> Key {
    return impl::KeyDecoder{text}.decodeConsoleInput();
}

auto Key::toString() const -> String {
    auto builder = StringEditor{};
    appendModifierString(builder, _modifiers);
    if (_type == Character || _type == Combined) {
        builder.append(_character.toString());
        return builder;
    }
    if (const auto definition = findKeyTextDefinition(_type)) {
        builder.append(definition->text);
        return builder;
    }
    return {};
}

auto Key::toDisplayText(const bool useBrackets) const -> String {
    auto builder = StringEditor{};
    appendModifierDisplayText(builder, _modifiers);
    if (_type == Character || _type == Combined) {
        builder.append(_character.toString());
        return wrapDisplayText(builder, useBrackets);
    }
    if (const auto definition = findKeyTextDefinition(_type)) {
        builder.append(definition->displayText);
        return wrapDisplayText(builder, useBrackets);
    }
    return {};
}

}
