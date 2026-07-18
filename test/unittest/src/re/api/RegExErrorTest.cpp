// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"

#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <type_traits>

using namespace el::re;
using namespace el::text::literals;

TESTED_TARGETS(RegExError RegExErrorContext RegExErrorDiagnostic)
TAGS(Api Errors)
class RegExErrorTest final : public el::UnitTest {
private:
    void requireContains(const el::text::String &text, const std::string_view expected) {
        const auto actual = re_test::string_helper::toStdString(text);
        REQUIRE(actual.find(expected) != std::string::npos);
    }

public:
    void testRuntimeErrorAndSimpleConstructor() {
        static_assert(std::is_base_of_v<el::err::RuntimeError, RegExError>);
        const auto error = RegExError{ErrorCategory::Parser, "Pattern is invalid"_el};

        REQUIRE_EQUAL(error.category(), ErrorCategory::Parser);
        REQUIRE_EQUAL(error.title(), "Pattern is invalid"_el);
        REQUIRE_EQUAL(error.reason(), "Pattern is invalid"_el);
        REQUIRE_EQUAL(std::string{error.what()}, std::string{"Pattern is invalid"});
        REQUIRE(error.description().isEmpty());
        REQUIRE(error.line().isNoIndex());
        REQUIRE(error.column().isNoIndex());
        REQUIRE(error.position().isNoIndex());
    }

    void testCompleteContextAndConvenienceAccessors() {
        const auto location = el::unit::CodeLocation{
            .line = el::unit::LineIndex{2U}, .column = el::unit::ColumnIndex{4U}, .position = el::unit::CpIndex{8U}};
        const auto error = RegExError{RegExErrorContext{
            ErrorCategory::Assembler, "Instruction is invalid"_el, "The operation name is not defined."_el, location}};

        REQUIRE_EQUAL(error.context().category(), ErrorCategory::Assembler);
        REQUIRE_EQUAL(error.description(), "The operation name is not defined."_el);
        REQUIRE_EQUAL(error.location().line, location.line);
        REQUIRE_EQUAL(error.line(), location.line);
        REQUIRE_EQUAL(error.column(), location.column);
        REQUIRE_EQUAL(error.position(), location.position);

        const auto updated = error.withLineNumber(el::unit::LineIndex{7U});
        REQUIRE_EQUAL(updated.line(), el::unit::LineIndex{7U});
        REQUIRE_EQUAL(updated.column(), location.column);
        REQUIRE_EQUAL(error.line(), location.line);
    }

    void testDiagnosticContainsAllDetails() {
        const auto error = RegExError{
            ErrorCategory::Format,
            "Replacement expression is invalid"_el,
            "A closing brace has no matching opening brace."_el,
            {.line = el::unit::LineIndex{1U}, .column = el::unit::ColumnIndex{2U}, .position = el::unit::CpIndex{3U}}};
        const auto diagnostic = error.diagnostic();

        REQUIRE(diagnostic != nullptr);
        REQUIRE_EQUAL(diagnostic->location().position, el::unit::CpIndex{3U});
        WITH_CONTEXT(requireContains(diagnostic->toString(), "Replacement expression is invalid"));
        WITH_CONTEXT(requireContains(diagnostic->toString(), "A closing brace has no matching opening brace."));
        WITH_CONTEXT(requireContains(diagnostic->toString(), "category"));
        WITH_CONTEXT(requireContains(diagnostic->toString(), "Format"));
        WITH_CONTEXT(requireContains(diagnostic->toString(), "Line"));
        WITH_CONTEXT(requireContains(diagnostic->toString(), "Column"));
        WITH_CONTEXT(requireContains(diagnostic->toString(), "Position"));
    }

    void testPlainStringUsesConciseSingleLineFormat() {
        const auto error = RegExError{
            ErrorCategory::Assembler,
            "Failed to assemble regular expression"_el,
            "The operation name is not defined."_el};

        REQUIRE_EQUAL(
            error.toString(),
            "Assembler: Failed to assemble regular expression. The operation name is not defined."_el);
    }
};
