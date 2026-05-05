// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(KeyModifiers)
class KeyModifiersTest final : public UNITTEST_SUBCLASS(TestHelper) {
public:
    void testDefaultConstructor() {
        const auto modifiers = KeyModifiers{};

        REQUIRE(modifiers.empty());
        REQUIRE_FALSE(modifiers.has(KeyModifier::Shift));
        REQUIRE_FALSE(modifiers.has(KeyModifier::Control));
        REQUIRE_FALSE(modifiers.has(KeyModifier::Alt));
        REQUIRE_EQUAL(modifiers.mask(), KeyModifiers::Mask{0});
    }

    void testConstructionAndCombination() {
        const auto modifiers = KeyModifier::Shift | KeyModifier::Control | KeyModifier::Alt;

        REQUIRE_FALSE(modifiers.empty());
        REQUIRE(modifiers.has(KeyModifier::Shift));
        REQUIRE(modifiers.has(KeyModifier::Control));
        REQUIRE(modifiers.has(KeyModifier::Alt));
        REQUIRE_EQUAL(modifiers, (KeyModifiers{KeyModifier::Shift} | KeyModifier::Control | KeyModifier::Alt));
    }

    void testSetAndClear() {
        auto modifiers = KeyModifiers{};

        modifiers.set(KeyModifier::Shift);
        modifiers.set(KeyModifier::Alt);
        REQUIRE(modifiers.has(KeyModifier::Shift));
        REQUIRE(modifiers.has(KeyModifier::Alt));

        modifiers.clear(KeyModifier::Shift);
        REQUIRE_FALSE(modifiers.has(KeyModifier::Shift));
        REQUIRE(modifiers.has(KeyModifier::Alt));

        modifiers.set(KeyModifier::Alt, false);
        REQUIRE(modifiers.empty());
    }
};
