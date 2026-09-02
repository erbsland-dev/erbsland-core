// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/DependencyMode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <format>

using namespace el::conf;
using namespace el::text::literals;
using namespace el::conf::vr;

TESTED_TARGETS(DependencyMode)
class DependencyModeTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testDefaultConstructor() {
        const DependencyMode mode;

        REQUIRE_EQUAL(mode, DependencyMode::Undefined);
        REQUIRE_EQUAL(mode.raw(), DependencyMode::Undefined);
    }

    void testEnumConstructorAndComparison() {
        const DependencyMode ifMode{DependencyMode::If};

        REQUIRE_EQUAL(ifMode, DependencyMode::If);
        REQUIRE_EQUAL(DependencyMode::If, ifMode);
        REQUIRE_NOT_EQUAL(ifMode, DependencyMode::IfNot);
        REQUIRE_EQUAL(ifMode.raw(), DependencyMode::If);
    }

    void testAssignmentFromEnum() {
        DependencyMode mode;
        mode = DependencyMode::XOR;

        REQUIRE_EQUAL(mode, DependencyMode::XOR);
        REQUIRE_EQUAL(mode.raw(), DependencyMode::XOR);
    }

    void testToText() {
        const std::array<std::pair<DependencyMode::Enum, el::text::String>, 6> mappings = {
            std::pair{DependencyMode::If, el::text::String{"if"_el}},
            std::pair{DependencyMode::IfNot, el::text::String{"if_not"_el}},
            std::pair{DependencyMode::OR, el::text::String{"or"_el}},
            std::pair{DependencyMode::XNOR, el::text::String{"xnor"_el}},
            std::pair{DependencyMode::XOR, el::text::String{"xor"_el}},
            std::pair{DependencyMode::AND, el::text::String{"and"_el}},
        };
        for (const auto &[mode, expectedText] : mappings) {
            const auto actualText = DependencyMode{mode}.toText();
            REQUIRE_EQUAL(actualText, expectedText);
        }

        const auto undefinedText = DependencyMode{}.toText();
        REQUIRE_EQUAL(undefinedText, el::text::String{"undefined"_el});
    }

    void testFromText() {
        const auto ifMode = DependencyMode::fromText("if"_el);
        const auto uppercaseIfMode = DependencyMode::fromText("IF"_el);
        const auto ifNotMode = DependencyMode::fromText("if_not"_el);
        const auto spacedIfNotMode = DependencyMode::fromText("if not"_el);
        const auto orMode = DependencyMode::fromText("OR"_el);
        const auto xnorMode = DependencyMode::fromText("xNoR"_el);
        const auto xorMode = DependencyMode::fromText("xor"_el);
        const auto andMode = DependencyMode::fromText("aNd"_el);
        const auto emptyMode = DependencyMode::fromText(el::text::String{});
        const auto unknownMode = DependencyMode::fromText("unknown"_el);
        const auto longUnknownMode = DependencyMode::fromText("123456789012345678901"_el);
        REQUIRE_EQUAL(ifMode, DependencyMode::If);
        REQUIRE_EQUAL(uppercaseIfMode, DependencyMode::If);
        REQUIRE_EQUAL(ifNotMode, DependencyMode::IfNot);
        REQUIRE_EQUAL(spacedIfNotMode, DependencyMode::IfNot);
        REQUIRE_EQUAL(orMode, DependencyMode::OR);
        REQUIRE_EQUAL(xnorMode, DependencyMode::XNOR);
        REQUIRE_EQUAL(xorMode, DependencyMode::XOR);
        REQUIRE_EQUAL(andMode, DependencyMode::AND);
        REQUIRE_EQUAL(emptyMode, DependencyMode::Undefined);
        REQUIRE_EQUAL(unknownMode, DependencyMode::Undefined);
        REQUIRE_EQUAL(longUnknownMode, DependencyMode::Undefined);
    }

    void testFormatter() {
        const auto formatted = std::format("{}", DependencyMode{DependencyMode::IfNot});
        REQUIRE_EQUAL(formatted, "if_not");
    }
};
