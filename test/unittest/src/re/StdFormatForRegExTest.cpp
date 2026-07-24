// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>

TESTED_TARGETS(StdFormatForRegEx)
class StdFormatForRegExTest final : public el::UnitTest {
public:
    void testPublicValues() {
        REQUIRE_EQUAL(std::format("{}", el::re::ErrorCategory::Parser), std::string{"Parser"});
        REQUIRE_EQUAL(std::format("{}", el::re::Flag::IgnoreCase), std::string{"IgnoreCase"});
        REQUIRE_EQUAL(
            std::format("{}", el::re::Flags{el::re::Flag::IgnoreCase, el::re::Flag::Multiline}),
            std::string{"IgnoreCase, Multiline"});
        REQUIRE_EQUAL(std::format("{}", el::re::CaptureRange{2, 7}), std::string{"2-7"});
    }

    void testImplementationValues() {
        REQUIRE_EQUAL(std::format("{}", el::re::impl::ArgumentKind::ProgramCounter), std::string{"Program Counter"});
        REQUIRE_EQUAL(std::format("{}", el::re::impl::DataSection::Program), std::string{"program"});
        REQUIRE_EQUAL(std::format("{}", el::re::impl::OperationModifier::Negated), std::string{"NOT"});
    }
};
