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
        const auto errorCategory = std::format("{}", el::re::ErrorCategory::Parser);
        const auto flag = std::format("{}", el::re::Flag::IgnoreCase);
        const auto flags = std::format("{}", el::re::Flags{el::re::Flag::IgnoreCase, el::re::Flag::Multiline});
        const auto captureRange = std::format("{}", el::re::CaptureRange{2, 7});
        REQUIRE_EQUAL(errorCategory, std::string{"Parser"});
        REQUIRE_EQUAL(flag, std::string{"IgnoreCase"});
        REQUIRE_EQUAL(flags, std::string{"IgnoreCase, Multiline"});
        REQUIRE_EQUAL(captureRange, std::string{"2-7"});
    }

    void testImplementationValues() {
        const auto argumentKind = std::format("{}", el::re::impl::ArgumentKind::ProgramCounter);
        const auto dataSection = std::format("{}", el::re::impl::DataSection::Program);
        const auto operationModifier = std::format("{}", el::re::impl::OperationModifier::Negated);
        REQUIRE_EQUAL(argumentKind, std::string{"Program Counter"});
        REQUIRE_EQUAL(dataSection, std::string{"program"});
        REQUIRE_EQUAL(operationModifier, std::string{"NOT"});
    }
};
