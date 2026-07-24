// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/system/UserLookup.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(UserLookup)
class PosixUserLookupTest final : public el::UnitTest {
public:
    void testRejectsInvalidIdentifiers() {
        auto lookup = el::system::UserLookup{};

        for (const auto id : {"+1"_el, "-1"_el, " 1"_el, "1 "_el, "1x"_el, "4294967296"_el}) {
            REQUIRE_THROWS_AS(el::err::ParameterError, lookup.userNameForId(el::system::UserId{id}));
            REQUIRE_THROWS_AS(el::err::ParameterError, lookup.groupNameForId(el::system::GroupId{id}));
        }
    }
};
