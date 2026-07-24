// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/vr/DependencyMode.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <format>

using namespace el::conf;
using namespace el::text::literals;
using namespace el::conf::impl;

TESTED_TARGETS(DependencyMode)
class DependencyModeTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testDefaultConstructor() {
        const DependencyMode mode;

        REQUIRE(mode == DependencyMode::Undefined);
        REQUIRE(mode.raw() == DependencyMode::Undefined);
    }

    void testEnumConstructorAndComparison() {
        const DependencyMode ifMode{DependencyMode::If};

        REQUIRE(ifMode == DependencyMode::If);
        REQUIRE(DependencyMode::If == ifMode);
        REQUIRE(ifMode != DependencyMode::IfNot);
        REQUIRE(ifMode.raw() == DependencyMode::If);
    }

    void testAssignmentFromEnum() {
        DependencyMode mode;
        mode = DependencyMode::XOR;

        REQUIRE(mode == DependencyMode::XOR);
        REQUIRE(mode.raw() == DependencyMode::XOR);
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
            REQUIRE_EQUAL(DependencyMode{mode}.toText(), expectedText);
        }

        REQUIRE_EQUAL(DependencyMode{}.toText(), el::text::String{"undefined"_el});
    }

    void testFromText() {
        REQUIRE(DependencyMode::fromText("if"_el) == DependencyMode::If);
        REQUIRE(DependencyMode::fromText("IF"_el) == DependencyMode::If);
        REQUIRE(DependencyMode::fromText("if_not"_el) == DependencyMode::IfNot);
        REQUIRE(DependencyMode::fromText("if not"_el) == DependencyMode::IfNot);
        REQUIRE(DependencyMode::fromText("OR"_el) == DependencyMode::OR);
        REQUIRE(DependencyMode::fromText("xNoR"_el) == DependencyMode::XNOR);
        REQUIRE(DependencyMode::fromText("xor"_el) == DependencyMode::XOR);
        REQUIRE(DependencyMode::fromText("aNd"_el) == DependencyMode::AND);

        REQUIRE(DependencyMode::fromText(el::text::String{}) == DependencyMode::Undefined);
        REQUIRE(DependencyMode::fromText("unknown"_el) == DependencyMode::Undefined);
        REQUIRE(DependencyMode::fromText("123456789012345678901"_el) == DependencyMode::Undefined);
    }

    void testFormatter() { REQUIRE_EQUAL(std::format("{}", DependencyMode{DependencyMode::IfNot}), "if_not"); }
};
