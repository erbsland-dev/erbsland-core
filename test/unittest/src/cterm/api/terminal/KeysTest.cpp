// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <stdexcept>
#include <vector>

TESTED_TARGETS(Keys)
class KeysTest final : public UNITTEST_SUBCLASS(TestHelper) {
public:
    void testConstructorsAndMatching() {
        auto singleCharacter = Keys{U'x'};
        REQUIRE_EQUAL(singleCharacter.keys(), (std::vector<Key>{Key{Key::Character, U'x'}}));
        REQUIRE(singleCharacter.contains(Key{Key::Character, U'x'}));
        REQUIRE(singleCharacter.matches(Key{Key::Character, U'x'}));
        REQUIRE_FALSE(singleCharacter.contains(Key{Key::Character, U'y'}));

        auto singleSpecial = Keys{Key::Enter};
        REQUIRE_EQUAL(singleSpecial.keys(), (std::vector<Key>{Key{Key::Enter}}));
        REQUIRE(singleSpecial.matches(Key{Key::Enter}));

        auto fromVector = Keys{std::vector<Key>{Key{Key::Tab}, Key{Key::Escape}}};
        REQUIRE_EQUAL(fromVector.keys(), (std::vector<Key>{Key{Key::Tab}, Key{Key::Escape}}));
    }

    void testUniqueOrderPreservesFirstOccurrence() {
        auto keys = Keys{
            Key{Key::Enter}, Key{Key::Character, U'x'}, Key{Key::Enter}, Key{Key::Character, U'x'}, Key{Key::Escape}};

        REQUIRE_EQUAL(keys.keys(), (std::vector<Key>{Key{Key::Enter}, Key{Key::Character, U'x'}, Key{Key::Escape}}));
    }

    void testAddAndClear() {
        auto keys = Keys{};
        keys.add(Key::PageUp).add(U'a').add(Key{Key::Combined, U"e\u0301"_el});

        REQUIRE_EQUAL(
            keys.keys(),
            (std::vector<Key>{Key{Key::PageUp}, Key{Key::Character, U'a'}, Key{Key::Combined, U"e\u0301"_el}}));

        keys.clear().add(Key::PageDown);
        REQUIRE_EQUAL(keys.keys(), (std::vector<Key>{Key{Key::PageDown}}));
        REQUIRE_EQUAL(keys.mainKeys(), (std::vector<Key>{Key{Key::PageDown}}));
        REQUIRE(keys.alternativeKeys().empty());
    }

    void testRejectsInvalidAndUndisplayableKeys() {
        REQUIRE_THROWS_AS(std::invalid_argument, Keys{Key::None});
        REQUIRE_THROWS_AS(std::invalid_argument, Keys{Key::Character});
        REQUIRE_THROWS_AS(std::invalid_argument, Keys{Key::Combined});

        auto keys = Keys{};
        REQUIRE_THROWS_AS(std::invalid_argument, keys.add(Key::None));
        REQUIRE_THROWS_AS(std::invalid_argument, keys.add(Key::Character));
        REQUIRE_THROWS_AS(std::invalid_argument, keys.setKeys({Key::None}));
    }

    void testMainAndAlternativeKeys() {
        auto keys = Keys{Key::Up, Key::Down, Key::PageUp, Key::PageDown, Key::Space, Key::Enter};
        keys.setMainKeyCount(4);

        REQUIRE_EQUAL(
            keys.mainKeys(), (std::vector<Key>{Key{Key::Up}, Key{Key::Down}, Key{Key::PageUp}, Key{Key::PageDown}}));
        REQUIRE_EQUAL(keys.alternativeKeys(), (std::vector<Key>{Key{Key::Space}, Key{Key::Enter}}));

        keys.setMainKeyCount(99);
        REQUIRE_EQUAL(keys.mainKeys(), keys.keys());
        REQUIRE(keys.alternativeKeys().empty());
    }
};
