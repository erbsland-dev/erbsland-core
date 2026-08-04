// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/engine/Operation.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <format>

using namespace el::re;
using impl::Operation;

TESTED_TARGETS(Operation)
TAGS(Matching)
class OperationTest final : public el::UnitTest {
public:
    void testBasics() {
        Operation op; // default constructed -> None
        REQUIRE_EQUAL(op.raw(), Operation::None);
        REQUIRE_FALSE(op.isFlow());
    }

    void testCodeConversionRoundTrip() {
        std::array<Operation::Value, 25> values{
            Operation::None,
            Operation::Split,
            Operation::Jump,
            Operation::Match,
            Operation::NotMatch,
            Operation::Anchor,
            Operation::StartCapture,
            Operation::Counter,
            Operation::Maximum,
            Operation::Minimum,
            Operation::Success,
            Operation::Failure,
            Operation::Char,
            Operation::CiChar,
            Operation::NotChar,
            Operation::NotCiChar,
            Operation::Sequence,
            Operation::CiSequence,
            Operation::Category,
            Operation::NotCategory,
            Operation::Class,
            Operation::CiClass,
            Operation::NotClass,
            Operation::NotCiClass,
            Operation::Any,
        };
        for (auto v : values) {
            Operation op{v};
            auto code = op.toCode();
            // pure round trip
            const auto roundTripped = Operation::fromCode(code);
            REQUIRE_EQUAL(roundTripped, op);
            // lower 24 bits ignored
            const auto maskedRoundTripped = Operation::fromCode(code | 0x00ffffffU);
            REQUIRE_EQUAL(maskedRoundTripped, op);
        }
    }

    void testEquality() {
        Operation a{Operation::Jump};
        Operation b{Operation::Jump};
        Operation c{Operation::Split};
        REQUIRE_EQUAL(a, b);
        REQUIRE_NOT_EQUAL(a, c);
    }

    void testMasksAndKinds() {
        // Flow ops
        REQUIRE(Operation{Operation::Split}.isFlow());
        REQUIRE(Operation{Operation::Jump}.isFlow());
        REQUIRE(Operation{Operation::Match}.isFlow());
        REQUIRE(Operation{Operation::NotMatch}.isFlow());
        REQUIRE(Operation{Operation::Success}.isFlow());
        REQUIRE(Operation{Operation::Failure}.isFlow());
        REQUIRE(Operation{Operation::Anchor}.isFlow());
        REQUIRE(Operation{Operation::StartCapture}.isFlow());
        REQUIRE(Operation{Operation::Counter}.isFlow());
        REQUIRE(Operation{Operation::Maximum}.isFlow());
        REQUIRE(Operation{Operation::Minimum}.isFlow());

        // Non-flow ops
        REQUIRE_FALSE(Operation{Operation::Char}.isFlow());
        REQUIRE_FALSE(Operation{Operation::CiChar}.isFlow());
        REQUIRE_FALSE(Operation{Operation::NotChar}.isFlow());
        REQUIRE_FALSE(Operation{Operation::NotCiChar}.isFlow());
        REQUIRE_FALSE(Operation{Operation::Sequence}.isFlow());
        REQUIRE_FALSE(Operation{Operation::CiSequence}.isFlow());
        REQUIRE_FALSE(Operation{Operation::Category}.isFlow());
        REQUIRE_FALSE(Operation{Operation::NotCategory}.isFlow());
        REQUIRE_FALSE(Operation{Operation::Class}.isFlow());
        REQUIRE_FALSE(Operation{Operation::CiClass}.isFlow());
        REQUIRE_FALSE(Operation{Operation::NotClass}.isFlow());
        REQUIRE_FALSE(Operation{Operation::NotCiClass}.isFlow());
        REQUIRE_FALSE(Operation{Operation::Any}.isFlow());

        // Size2 ops
        REQUIRE(Operation{Operation::Jump}.isSize2());
        REQUIRE_FALSE(Operation{Operation::Split}.isSize2());

        // Case-insensitive ops
        REQUIRE(Operation{Operation::CiChar}.isCaseInsensitive());
        REQUIRE(Operation{Operation::CiSequence}.isCaseInsensitive());
        REQUIRE(Operation{Operation::CiClass}.isCaseInsensitive());
        REQUIRE(Operation{Operation::NotCiChar}.isCaseInsensitive());
        REQUIRE(Operation{Operation::NotCiClass}.isCaseInsensitive());
        REQUIRE_FALSE(Operation{Operation::Char}.isCaseInsensitive());
        REQUIRE_FALSE(Operation{Operation::Sequence}.isCaseInsensitive());
        REQUIRE_FALSE(Operation{Operation::Class}.isCaseInsensitive());

        // Negated ops (flag is encoded into the opcode itself).
        REQUIRE(
            (static_cast<Operation::ValueNative>(Operation{Operation::NotMatch}.raw()) &
                static_cast<Operation::ValueNative>(Operation::NotMask)) != 0);
        REQUIRE(
            (static_cast<Operation::ValueNative>(Operation{Operation::Failure}.raw()) &
                static_cast<Operation::ValueNative>(Operation::NotMask)) != 0);
        REQUIRE(
            (static_cast<Operation::ValueNative>(Operation{Operation::NotChar}.raw()) &
                static_cast<Operation::ValueNative>(Operation::NotMask)) != 0);
        REQUIRE(
            (static_cast<Operation::ValueNative>(Operation{Operation::NotCategory}.raw()) &
                static_cast<Operation::ValueNative>(Operation::NotMask)) != 0);
        REQUIRE(
            (static_cast<Operation::ValueNative>(Operation{Operation::NotClass}.raw()) &
                static_cast<Operation::ValueNative>(Operation::NotMask)) != 0);
    }
};
