// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(TerminalFlags)
class TerminalFlagsTest final : public el::UnitTest {
public:
    void testFlagsCanBeSetClearedAndQueried() {
        auto flags = TerminalFlags{};

        REQUIRE_FALSE(flags.has(TerminalFlag::NoSignalHandling));

        flags.set(TerminalFlag::NoSignalHandling);
        REQUIRE(flags.has(TerminalFlag::NoSignalHandling));

        flags.set(TerminalFlag::NoSignalHandling, false);
        REQUIRE_FALSE(flags.has(TerminalFlag::NoSignalHandling));

        flags.set(TerminalFlag::NoSignalHandling);
        flags.clear(TerminalFlag::NoSignalHandling);
        REQUIRE_FALSE(flags.has(TerminalFlag::NoSignalHandling));
    }

    void testBitwiseOperatorsProduceEquivalentCombinedFlags() {
        const auto fromCtor = TerminalFlags{TerminalFlag::NoSignalHandling};
        const auto fromMemberOperator = TerminalFlags{} | TerminalFlag::NoSignalHandling;
        const auto fromFlagAndFlags = TerminalFlag::NoSignalHandling | TerminalFlags{};
        const auto fromTwoFlags = TerminalFlag::NoSignalHandling | TerminalFlag::NoSignalHandling;
        const auto fromTwoSets = fromCtor | fromMemberOperator;

        REQUIRE(fromCtor.has(TerminalFlag::NoSignalHandling));
        REQUIRE(fromMemberOperator.has(TerminalFlag::NoSignalHandling));
        REQUIRE(fromFlagAndFlags.has(TerminalFlag::NoSignalHandling));
        REQUIRE(fromTwoFlags.has(TerminalFlag::NoSignalHandling));
        REQUIRE(fromTwoSets.has(TerminalFlag::NoSignalHandling));
    }
};
