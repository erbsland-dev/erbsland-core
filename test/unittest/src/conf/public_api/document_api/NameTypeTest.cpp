// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../ConfTestHelper.hpp"

#include <erbsland/conf/NameType.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;

TESTED_TARGETS(NameType)
class NameTypeTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    Name name;

    void testFormat() {
        const auto regular = std::format("*{}*", NameType::Regular);
        const auto text = std::format("*{}*", NameType::Text);
        const auto index = std::format("*{}*", NameType::Index);
        const auto textIndex = std::format("*{}*", NameType::TextIndex);
        REQUIRE_EQUAL(regular, "*Regular*");
        REQUIRE_EQUAL(text, "*Text*");
        REQUIRE_EQUAL(index, "*Index*");
        REQUIRE_EQUAL(textIndex, "*TextIndex*");
    }
};
