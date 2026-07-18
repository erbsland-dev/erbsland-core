// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/impl/KeyDecoder.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <format>
#include <functional>
#include <string_view>

namespace th = erbsland::unittest::th;

TESTED_TARGETS(Key)
class KeyTest final : public UNITTEST_SUBCLASS(TestHelper) {
public:
    void testDefaultConstructor() {
        Key key;
        REQUIRE_EQUAL(key.type(), Key::None);
        REQUIRE_EQUAL(key.character(), 0);
        REQUIRE_EQUAL(key.unicode(), 0);
        REQUIRE_EQUAL(key.combined(), erbsland::text::U32StringEditor{});
        REQUIRE(!key.valid());
    }

    void testParameterizedConstructorAndEquality() {
        Key key{Key::Character, U'a'};
        REQUIRE_EQUAL(key.type(), Key::Character);
        REQUIRE(key.modifiers().empty());
        REQUIRE_EQUAL(key.character(), 'a');
        REQUIRE_EQUAL(key.unicode(), U'a');
        REQUIRE_EQUAL(key.combined(), U"a"_el);
        REQUIRE(key.valid());
        REQUIRE(key == Key{Key::Character, U'a'});
        REQUIRE(key != Key{Key::Character, U'b'});

        const auto combined = Key{Key::Combined, U"e\u0301"_el};
        REQUIRE_EQUAL(combined.type(), Key::Combined);
        REQUIRE_EQUAL(combined.character(), 0);
        REQUIRE_EQUAL(combined.unicode(), 0);
        REQUIRE_EQUAL(combined.combined(), U"e\u0301"_el);

        const auto shiftedUp = Key{Key::Up, KeyModifier::Shift};
        REQUIRE_EQUAL(shiftedUp.type(), Key::Up);
        REQUIRE(shiftedUp.hasModifier(KeyModifier::Shift));
        REQUIRE(shiftedUp != Key{Key::Up});
        REQUIRE(shiftedUp != Key::Up);
        REQUIRE_EQUAL(shiftedUp.withoutModifiers(), Key{Key::Up});
    }

    void testFromConsoleInputSpecialKeys() {
        struct Entry {
            const char *input;
            Key expected;
        };
        const std::array<Entry, 32> entries{{
            {"\n", Key{Key::Enter}},
            {"\r", Key{Key::Enter}},
            {"\t", Key{Key::Tab}},
            {"\x1b[Z", Key{Key::BackTab}},
            {" ", Key{Key::Space}},
            {"\x1b", Key{Key::Escape}},
            {"\x08", Key{Key::Backspace}},
            {"\x7f", Key{Key::Backspace}},
            {"\x1b[A", Key{Key::Up}},
            {"\x1b[B", Key{Key::Down}},
            {"\x1b[C", Key{Key::Right}},
            {"\x1b[D", Key{Key::Left}},
            {"\x1b[H", Key{Key::Home}},
            {"\x1b[F", Key{Key::End}},
            {"\x1bOH", Key{Key::Home}},
            {"\x1bOF", Key{Key::End}},
            {"\x1b[2~", Key{Key::Insert}},
            {"\x1b[3~", Key{Key::Delete}},
            {"\x1b[5~", Key{Key::PageUp}},
            {"\x1b[6~", Key{Key::PageDown}},
            {"\x1bOP", Key{Key::F1}},
            {"\x1bOQ", Key{Key::F2}},
            {"\x1bOR", Key{Key::F3}},
            {"\x1bOS", Key{Key::F4}},
            {"\x1b[15~", Key{Key::F5}},
            {"\x1b[17~", Key{Key::F6}},
            {"\x1b[18~", Key{Key::F7}},
            {"\x1b[19~", Key{Key::F8}},
            {"\x1b[20~", Key{Key::F9}},
            {"\x1b[21~", Key{Key::F10}},
            {"\x1b[23~", Key{Key::F11}},
            {"\x1b[24~", Key{Key::F12}},
        }};
        for (std::size_t index = 0; index < entries.size(); ++index) {
            const auto &entry = entries[index];
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    const auto key = Key::fromConsoleInput(erbsland::text::StringEditor{std::string_view{entry.input}});
                    REQUIRE_EQUAL(key, entry.expected);
                    REQUIRE(key.valid());
                },
                [&]() -> std::string {
                    return std::format(
                        "index = {} / inputSize = {} / expectedKey = {}",
                        index,
                        std::string_view{entry.input}.size(),
                        erbsland::text::StringConverter{entry.expected.toString()}.toStdString());
                });
        }
    }

    void testFromConsoleInputPrintableCharacters() {
        struct Entry {
            std::string input;
            Key expected;
        };
        const std::array<Entry, 7> entries{{
            {"x", Key{Key::Character, U'x'}},
            {"X", Key{Key::Character, U'X'}},
            {"#", Key{Key::Character, U'#'}},
            {"+", Key{Key::Character, U'+'}},
            {"?", Key{Key::Character, U'?'}},
            {bytes({0xC3, 0xA4}), Key{Key::Character, U'\u00e4'}},
            {bytes({0x65, 0xCC, 0x81}), Key{Key::Combined, U"e\u0301"_el}},
        }};
        for (std::size_t index = 0; index < entries.size(); ++index) {
            const auto &entry = entries[index];
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    const auto key = Key::fromConsoleInput(erbsland::text::StringEditor{entry.input});
                    REQUIRE_EQUAL(key, entry.expected);
                    REQUIRE(key.valid());
                },
                [&]() -> std::string { return std::format("index = {} / input = \"{}\"", index, entry.input); });
        }
    }

    void testFromConsoleInputInvalid() {
        auto key = Key::fromConsoleInput(""_el);
        REQUIRE_EQUAL(key, Key{Key::None});
        REQUIRE(!key.valid());

        key = Key::fromConsoleInput("ab"_el);
        REQUIRE_EQUAL(key, Key{Key::None});
        REQUIRE(!key.valid());

        key = Key::fromConsoleInput("\x01"_el);
        REQUIRE_EQUAL(key, Key{Key::None});
        REQUIRE(!key.valid());

        key = Key::fromConsoleInput("\x1b[999~"_el);
        REQUIRE_EQUAL(key, Key{Key::None});
        REQUIRE(!key.valid());
    }

    void testConsoleInputPrefixNeedsMoreUtf8Data() {
        using erbsland::cterm::impl::KeyDecoder;
        using erbsland::cterm::impl::KeyParseStatus;

        auto result = KeyDecoder{erbsland::text::StringEditor{th::stdStringFromHex("F0 9F")}}.parseConsoleInputPrefix();
        REQUIRE_EQUAL(result.status(), KeyParseStatus::NeedMoreData);

        result = KeyDecoder{erbsland::text::StringEditor{th::stdStringFromHex("65 CC")}}.parseConsoleInputPrefix();
        REQUIRE_EQUAL(result.status(), KeyParseStatus::NeedMoreData);

        result = KeyDecoder{erbsland::text::StringEditor{th::stdStringFromHex("65 CC 81")}}.parseConsoleInputPrefix();
        REQUIRE_EQUAL(result.status(), KeyParseStatus::Parsed);
        REQUIRE_EQUAL(result.key(), (Key{Key::Combined, U"e\u0301"_el}));
    }

    void testFromStringAndToString() {
        struct Entry {
            std::string input;
            Key expected;
            std::string text;
        };
        const std::array<Entry, 16> entries{{
            {"Return", Key{Key::Enter}, "enter"},
            {"TAB", Key{Key::Tab}, "tab"},
            {"shift_tab", Key{Key::BackTab}, "backtab"},
            {"Esc", Key{Key::Escape}, "escape"},
            {"del", Key{Key::Delete}, "delete"},
            {"page_up", Key{Key::PageUp}, "pageup"},
            {"PgDn", Key{Key::PageDown}, "pagedown"},
            {"Left", Key{Key::Left}, "left"},
            {"F12", Key{Key::F12}, "f12"},
            {"shift+up", Key{Key::Up, KeyModifier::Shift}, "shift+up"},
            {"ctrl+pageup", Key{Key::PageUp, KeyModifier::Control}, "ctrl+pageup"},
            {"alt+f4", Key{Key::F4, KeyModifier::Alt}, "alt+f4"},
            {"X", Key{Key::Character, U'X'}, "X"},
            {"?", Key{Key::Character, U'?'}, "?"},
            {bytes({0xC3, 0xA4}), Key{Key::Character, U'\u00e4'}, bytes({0xC3, 0xA4})},
            {bytes({0x65, 0xCC, 0x81}), Key{Key::Combined, U"e\u0301"_el}, bytes({0x65, 0xCC, 0x81})},
        }};
        for (std::size_t index = 0; index < entries.size(); ++index) {
            const auto &entry = entries[index];
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    const auto key = Key::fromString(erbsland::text::StringEditor{entry.input});
                    REQUIRE_EQUAL(key, entry.expected);
                    REQUIRE_EQUAL(key.toString(), erbsland::text::StringEditor{entry.text});
                },
                [&]() -> std::string { return std::format("index = {} / input = \"{}\"", index, entry.input); });
        }

        REQUIRE_EQUAL(Key{}.toString(), ""_el);
        REQUIRE_EQUAL(Key::fromString("shift+x"_el), Key{Key::None});
    }

    void testLegacyAsciiAndUnicodeAccessors() {
        const auto ascii = Key{Key::Character, U'A'};
        const auto unicode = Key{Key::Character, U'\u00e4'};
        const auto combined = Key{Key::Combined, U"e\u0301"_el};

        REQUIRE_EQUAL(ascii.character(), 'A');
        REQUIRE_EQUAL(ascii.unicode(), U'A');
        REQUIRE_EQUAL(ascii.combined(), U"A"_el);

        REQUIRE_EQUAL(unicode.character(), 0);
        REQUIRE_EQUAL(unicode.unicode(), U'\u00e4');
        REQUIRE_EQUAL(unicode.combined(), U"\u00e4"_el);

        REQUIRE_EQUAL(combined.character(), 0);
        REQUIRE_EQUAL(combined.unicode(), 0);
        REQUIRE_EQUAL(combined.combined(), U"e\u0301"_el);
    }

    void testToDisplayText() {
        struct Entry {
            Key key;
            const char *withBrackets;
            const char *withoutBrackets;
        };
        const std::array<Entry, 12> entries{{
            {Key{Key::Enter}, "[↵]", "↵"},
            {Key{Key::Tab}, "[tab]", "tab"},
            {Key{Key::BackTab}, "[⇤]", "⇤"},
            {Key{Key::Escape}, "[esc]", "esc"},
            {Key{Key::Backspace}, "[⌫]", "⌫"},
            {Key{Key::Left}, "[←]", "←"},
            {Key{Key::PageDown}, "[pgdn]", "pgdn"},
            {Key{Key::F5}, "[F5]", "F5"},
            {Key{Key::Up, KeyModifier::Shift}, "[Shift+↑]", "Shift+↑"},
            {Key{Key::F4, KeyModifier::Control | KeyModifier::Alt}, "[Ctrl+Alt+F4]", "Ctrl+Alt+F4"},
            {Key{Key::Character, U'x'}, "[x]", "x"},
            {Key{Key::Combined, U"e\u0301"_el}, "[é]", "é"},
        }};
        for (std::size_t index = 0; index < entries.size(); ++index) {
            const auto &entry = entries[index];
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    REQUIRE_EQUAL(entry.key.toDisplayText(), erbsland::text::StringEditor{entry.withBrackets});
                    REQUIRE_EQUAL(entry.key.toDisplayText(false), erbsland::text::StringEditor{entry.withoutBrackets});
                },
                [&]() -> std::string {
                    return std::format(
                        "index = {} / expectedWithBrackets = \"{}\" / expectedWithoutBrackets = \"{}\"",
                        index,
                        entry.withBrackets,
                        entry.withoutBrackets);
                });
        }

        REQUIRE_EQUAL(Key{}.toDisplayText(), ""_el);
        REQUIRE_EQUAL(Key{}.toDisplayText(false), ""_el);
    }

    void testFromStringReplacesInvalidUtf8() {
        const auto key = Key::fromString(erbsland::text::StringEditor{bytes({0xC3})});

        REQUIRE_EQUAL(key, (Key{Key::Character, U'\uFFFD'}));
        REQUIRE(key.valid());
    }

    void testFromStringReplacesStructurallyUnsupportedText() {
        const auto multiple = Key::fromString("ab"_el);
        const auto control = Key::fromString("\x01"_el);

        REQUIRE_EQUAL(multiple, (Key{Key::Character, U'\uFFFD'}));
        REQUIRE(multiple.valid());
        REQUIRE_EQUAL(control, (Key{Key::Character, U'\uFFFD'}));
        REQUIRE(control.valid());
    }

    void testHashMatchesStdHashAndIncludesCharacterPayload() {
        const auto characterA = Key{Key::Character, U'a'};
        const auto characterB = Key{Key::Character, U'b'};
        const auto combined = Key{Key::Combined, U"e\u0301"_el};
        const auto special = Key{Key::Enter};
        const auto shiftedSpecial = Key{Key::Enter, KeyModifier::Shift};

        REQUIRE_EQUAL(characterA.hash(), std::hash<Key>{}(characterA));
        REQUIRE_EQUAL(special.hash(), std::hash<Key>{}(special));
        REQUIRE_EQUAL(combined.hash(), std::hash<Key>{}(combined));
        REQUIRE_NOT_EQUAL(characterA.hash(), characterB.hash());
        REQUIRE_NOT_EQUAL(characterA.hash(), special.hash());
        REQUIRE_NOT_EQUAL(combined.hash(), characterA.hash());
        REQUIRE_NOT_EQUAL(special.hash(), shiftedSpecial.hash());
    }
};
