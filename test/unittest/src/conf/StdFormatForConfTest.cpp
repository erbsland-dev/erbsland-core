// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>

using namespace el::text::literals;

TESTED_TARGETS(StdFormatForConf)
class StdFormatForConfTest final : public el::UnitTest {
public:
    void testPublicValues() {
        const auto errorCategory =
            std::format("{}", el::conf::ConfErrorCategory{el::conf::ConfErrorCategory::LimitExceeded});
        const auto nameType = std::format("{}", el::conf::NameType::Regular);
        const auto valueType = std::format("{}", el::conf::ValueType{el::conf::ValueType::Text});
        REQUIRE_EQUAL(errorCategory, std::string{"LimitExceeded"});
        REQUIRE_EQUAL(nameType, std::string{"Regular"});
        REQUIRE_EQUAL(valueType, std::string{"Text"});
    }

    void testImplementationValues() {
        const auto dependencyMode =
            std::format("{}", el::conf::impl::DependencyMode{el::conf::impl::DependencyMode::IfNot});
        const auto tokenType = std::format("{}", el::conf::impl::TokenType{el::conf::impl::TokenType::Text});
        const auto key = std::format("{}", el::conf::impl::ConfKey{"entry"_el});
        REQUIRE_EQUAL(dependencyMode, std::string{"if_not"});
        REQUIRE_EQUAL(tokenType, std::string{"Text"});
        REQUIRE_EQUAL(key, std::string{"entry"});
    }
};
