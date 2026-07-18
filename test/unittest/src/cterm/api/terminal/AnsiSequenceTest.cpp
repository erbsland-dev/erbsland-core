// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cterm/impl/AnsiSequence.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace erbsland::cterm::impl;

TESTED_TARGETS(ansi_sequence)
class AnsiSequenceTest final : public el::UnitTest {
public:
    void testColorSequences() {
        const auto escape = std::string{char{27}};
        REQUIRE_EQUAL(toStdString(ansi_sequence::color(31)), escape + "[31m");
        REQUIRE_EQUAL(toStdString(ansi_sequence::color(31, 44)), escape + "[31;44m");
    }

    void testCursorSequences() {
        const auto escape = std::string{char{27}};
        REQUIRE_EQUAL(toStdString(ansi_sequence::cursorPosition(3, 7)), escape + "[3;7H");
        REQUIRE_EQUAL(toStdString(ansi_sequence::moveLeft(2)), escape + "[2D");
        REQUIRE_EQUAL(toStdString(ansi_sequence::moveRight(4)), escape + "[4C");
        REQUIRE_EQUAL(toStdString(ansi_sequence::moveUp(6)), escape + "[6A");
        REQUIRE_EQUAL(toStdString(ansi_sequence::moveDown(8)), escape + "[8B");
    }

private:
    [[nodiscard]] static auto toStdString(const el::text::String &text) -> std::string {
        return el::text::StringConverter{text}.toStdString();
    }
};
