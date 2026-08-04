// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/char/NamedChars.hpp>
#include <erbsland/conf/impl/decoder/DecodedChar.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/unit/CodeLocation.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace erbsland::conf;
using el::conf::impl::DecodedChar;
using el::conf::impl::internalView;
namespace nc = erbsland::conf::impl::nc;

TESTED_TARGETS(DecodedChar)
class DecodedCharTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    DecodedChar character;

    void testDefaultConstructor() {
        character = {};
        REQUIRE(character.character().isEndOfData());
        REQUIRE_EQUAL(character.index(), el::unit::ByteIndex{});
        REQUIRE(character.codeLocation().isUndefined());
    }

    void testParameterizedConstructorAndAccessors() {
        el::unit::CodeLocation position{el::unit::LineIndex{2U}, el::unit::ColumnIndex{3U}};
        character = DecodedChar{U'A', el::unit::ByteIndex{7}, position};
        REQUIRE_EQUAL(character.character(), U'A');
        REQUIRE_EQUAL(character.index(), el::unit::ByteIndex{7});
        REQUIRE_EQUAL(character.codeLocation(), position);
    }

    void testCopyAndMove() {
        el::unit::CodeLocation position{el::unit::LineIndex{1U}, el::unit::ColumnIndex{4U}};
        character = DecodedChar{U'B', el::unit::ByteIndex{9}, position};
        DecodedChar copy{character};
        REQUIRE_EQUAL(copy.character(), character.character());
        REQUIRE_EQUAL(copy.index(), character.index());
        REQUIRE_EQUAL(copy.codeLocation(), character.codeLocation());
    }
};
