// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/diagnostics/Argument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using namespace el::text::literals;
using impl::ArgumentKind;
using impl::ArgumentType;
using impl::argumentTypeFromValue;
using impl::ArgumentValue;
using impl::toString;

TESTED_TARGETS(Argument)
TAGS(Diagnostics)
class ArgumentTest final : public el::UnitTest {
public:
    void testToStringArgumentKindKnownValues() {
        REQUIRE_EQUAL(toString(ArgumentKind::Unknown), "Unknown"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::ProgramCounter), "Program Counter"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::Char), "Char"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::CaptureGroup), "Capture Group"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::Anchor), "Anchor"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::Category), "Category"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::SequenceIndex), "Sequence Index"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::SequenceLength), "Sequence Length"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::CharClassIndex), "Char Class Index"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::CounterIndex), "Counter Index"_el);
        REQUIRE_EQUAL(toString(ArgumentKind::CounterValue), "Counter Value"_el);
    }

    void testToStringArgumentKindUnknownValue() {
        REQUIRE_EQUAL(toString(static_cast<ArgumentKind>(0xffU)), el::text::StringView{});
    }

    void testToStringArgumentTypeKnownValues() {
        REQUIRE_EQUAL(toString(ArgumentType::Text), "Text"_el);
        REQUIRE_EQUAL(toString(ArgumentType::Integer), "Integer"_el);
        REQUIRE_EQUAL(toString(ArgumentType::Boolean), "Boolean"_el);
    }

    void testToStringArgumentTypeUnknownValue() {
        REQUIRE_EQUAL(toString(static_cast<ArgumentType>(0xffU)), el::text::StringView{});
    }

    void testFormat() {
        REQUIRE_EQUAL(std::format("{}", ArgumentType::Text), "Text");
        REQUIRE_EQUAL(std::format("{}", ArgumentType::Integer), "Integer");
        REQUIRE_EQUAL(std::format("{}", ArgumentType::Boolean), "Boolean");

        REQUIRE_EQUAL(std::format("{}", ArgumentKind::ProgramCounter), "Program Counter");
    }
};
