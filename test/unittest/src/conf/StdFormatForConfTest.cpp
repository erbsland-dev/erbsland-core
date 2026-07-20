// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>

using namespace el::text::literals;

TESTED_TARGETS(StdFormatForConf)
class StdFormatForConfTest final : public el::UnitTest {
public:
    void testPublicValues() {
        REQUIRE_EQUAL(
            std::format("{}", el::conf::ConfErrorCategory{el::conf::ConfErrorCategory::LimitExceeded}),
            std::string{"LimitExceeded"});
        REQUIRE_EQUAL(std::format("{}", el::conf::NameType::Regular), std::string{"Regular"});
        REQUIRE_EQUAL(std::format("{}", el::conf::ValueType{el::conf::ValueType::Text}), std::string{"Text"});
    }

    void testImplementationValues() {
        REQUIRE_EQUAL(
            std::format("{}", el::conf::impl::DependencyMode{el::conf::impl::DependencyMode::IfNot}),
            std::string{"if_not"});
        REQUIRE_EQUAL(
            std::format("{}", el::conf::impl::TokenType{el::conf::impl::TokenType::Text}), std::string{"Text"});
        REQUIRE_EQUAL(std::format("{}", el::conf::impl::ConfKey{"entry"_el}), std::string{"entry"});
    }
};
