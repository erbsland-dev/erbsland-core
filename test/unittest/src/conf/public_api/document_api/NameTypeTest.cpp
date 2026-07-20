// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../ConfTestHelper.hpp"

#include <erbsland/conf/NameType.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;

TESTED_TARGETS(NameType)
class NameTypeTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    Name name;

    void testFormat() {
        REQUIRE_EQUAL(std::format("*{}*", NameType::Regular), "*Regular*");
        REQUIRE_EQUAL(std::format("*{}*", NameType::Text), "*Text*");
        REQUIRE_EQUAL(std::format("*{}*", NameType::Index), "*Index*");
        REQUIRE_EQUAL(std::format("*{}*", NameType::TextIndex), "*TextIndex*");
    }
};
