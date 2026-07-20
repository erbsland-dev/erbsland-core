// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/err/DiagnosticHelper.hpp>
#include <erbsland/err/Exception.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <exception>
#include <string>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(ConfError ConfErrorContext ConfErrorCategory)
class ConfErrorTest final : public el::UnitTest {
public:
    void testContextAccessorsAndOptionalFields() {
        auto context = ConfErrorContext{
            ConfErrorCategory::Syntax, "Parsing the Configuration Failed"_el, "An unexpected token was found."_el};
        REQUIRE_EQUAL(context.title(), "Parsing the Configuration Failed"_el);
        REQUIRE_EQUAL(context.description(), "An unexpected token was found."_el);
        REQUIRE_EQUAL(context.category(), ConfErrorCategory::Syntax);
        REQUIRE_FALSE(context.location().has_value());
        REQUIRE_FALSE(context.namePath().has_value());
        REQUIRE_FALSE(context.filePath().has_value());
        REQUIRE_FALSE(context.codeSnippet().has_value());

        context
            .setLocation(
                el::unit::CodeLocation{el::unit::LineIndex{1U}, el::unit::ColumnIndex{3U}, el::unit::CpIndex{9U}})
            .setNamePath(NamePath::fromText("main.value"_el))
            .setFilePath(el::path::Path{"example.elcl"_el});
        REQUIRE(context.location().has_value());
        REQUIRE(context.namePath().has_value());
        REQUIRE(context.filePath().has_value());
    }

    void testCopyEnrichmentDoesNotMutateTheOriginal() {
        const auto sourceIdentifier = SourceIdentifier::createForFile("example.elcl"_el);
        const auto location = Location{
            sourceIdentifier,
            el::unit::CodeLocation{el::unit::LineIndex{4U}, el::unit::ColumnIndex{2U}, el::unit::CpIndex{20U}}};
        const auto original =
            ConfErrorContext{ConfErrorCategory::Validation, "Validation Failed"_el, "The original description."_el};
        const auto enriched = original.withNamePathAndLocation(NamePath::fromText("main.value"_el), location)
                                  .withDescriptionPrefix("Additional context: "_el);

        REQUIRE_FALSE(original.location().has_value());
        REQUIRE_FALSE(original.namePath().has_value());
        REQUIRE_FALSE(original.filePath().has_value());
        REQUIRE_EQUAL(original.description(), "The original description."_el);
        REQUIRE(enriched.location().has_value());
        REQUIRE(enriched.namePath().has_value());
        REQUIRE(enriched.filePath().has_value());
        REQUIRE_EQUAL(enriched.description(), "Additional context: The original description."_el);
    }

    void testCodeSnippetEnrichmentAndCausePreservation() {
        auto lines = el::text::StringList{};
        lines.append("value: 1"_el);
        auto snippet = el::text::CodeSnippet{std::move(lines), el::unit::LineIndex{4U}, "elcl"_el};
        const auto cause = std::make_exception_ptr(el::err::Exception{"The source stream failed."_el});
        const auto error = ConfError{
            ConfErrorContext{ConfErrorCategory::IO, "Reading the Configuration Failed"_el, "Input failed."_el}, cause};

        const auto unchanged = error.withCodeSnippet(std::nullopt);
        REQUIRE_FALSE(unchanged.context().codeSnippet().has_value());
        REQUIRE(unchanged.cause() == cause);

        const auto enriched = error.withCodeSnippet(snippet);
        REQUIRE(enriched.context().codeSnippet().has_value());
        REQUIRE_EQUAL(enriched.context().codeSnippet()->language, "elcl"_el);
        REQUIRE(enriched.cause() == cause);
        REQUIRE_FALSE(error.context().codeSnippet().has_value());
    }

    void testSourceAwareContextCapturesTheLocationAndSnippet() {
        auto source = Source::fromString("zero\none\ntwo\nthree\n"_el);
        const auto location =
            Location{source->identifier(), el::unit::CodeLocation{el::unit::LineIndex{1U}, el::unit::ColumnIndex{1U}}};
        const auto context = ConfErrorContext{ConfErrorCategory::Syntax, "The value is invalid."_el, source, location};

        REQUIRE(context.location().has_value());
        REQUIRE_EQUAL(context.location()->line(), el::unit::LineIndex{1U});
        REQUIRE(context.codeSnippet().has_value());
        REQUIRE_EQUAL(context.codeSnippet()->startLine, el::unit::LineIndex::zero());
        REQUIRE_EQUAL(context.codeSnippet()->lines.count(), el::unit::ElementCount{4U});
    }

    void testExceptionUsesTheTitleAsItsReason() {
        const auto error = ConfError{ConfErrorContext{
            ConfErrorCategory::LimitExceeded,
            "A Configuration Limit Was Exceeded"_el,
            "The document contains too many values."_el}};
        REQUIRE_EQUAL(error.reason(), "A Configuration Limit Was Exceeded"_el);
        REQUIRE_EQUAL(error.toString(), "A Configuration Limit Was Exceeded"_el);
        REQUIRE_EQUAL(error.description(), "The document contains too many values."_el);
        REQUIRE(std::string{error.what()}.find("A Configuration Limit Was Exceeded") != std::string::npos);
    }

    void testPlainTextDiagnosticContainsAllContext() {
        auto lines = el::text::StringList{};
        lines.append("[main]"_el);
        lines.append("value: ???"_el);
        lines.append("next: 1"_el);
        auto context = ConfErrorContext{
            ConfErrorCategory::Syntax,
            "Parsing the Configuration Failed"_el,
            "Expected a value, but got an invalid token."_el};
        context
            .setLocation(
                el::unit::CodeLocation{el::unit::LineIndex{1U}, el::unit::ColumnIndex{7U}, el::unit::CpIndex{14U}})
            .setNamePath(NamePath::fromText("main.value"_el))
            .setFilePath(el::path::Path{"example.elcl"_el})
            .setCodeSnippet(el::text::CodeSnippet{std::move(lines), el::unit::LineIndex::zero(), "elcl"_el});
        const auto error = ConfError{std::move(context)};
        const auto rendered = el::text::StringConverter{error.diagnostic()->toString()}.toStdString();

        REQUIRE(rendered.find("Parsing the Configuration Failed") != std::string::npos);
        REQUIRE(rendered.find("Expected a value, but got an invalid token.") != std::string::npos);
        REQUIRE(rendered.find("Syntax") != std::string::npos);
        REQUIRE(rendered.find("main.value") != std::string::npos);
        REQUIRE(rendered.find("example.elcl") != std::string::npos);
        REQUIRE(rendered.find("Line:") != std::string::npos);
        REQUIRE(rendered.find("Column:") != std::string::npos);
        REQUIRE(rendered.find("2 │ value: ???") != std::string::npos);
        REQUIRE(rendered.find("▔") != std::string::npos);
    }

    void testDiagnosticClipsTheMarkerToTheAvailableLine() {
        auto lines = el::text::StringList{};
        lines.append("abc"_el);
        auto context = ConfErrorContext{ConfErrorCategory::Syntax, "Parsing Failed"_el, "Invalid input."_el};
        context.setLocation(el::unit::CodeLocation{el::unit::LineIndex::zero(), el::unit::ColumnIndex{99U}})
            .setCodeSnippet(el::text::CodeSnippet{std::move(lines), el::unit::LineIndex::zero(), "elcl"_el});
        const auto rendered =
            el::text::StringConverter{ConfError{std::move(context)}.diagnostic()->toString()}.toStdString();
        REQUIRE(rendered.find("1 │ abc") != std::string::npos);
        REQUIRE(rendered.find("│   ▔") != std::string::npos);
    }

    void testGenericDiagnosticHelperAppendsTheCause() {
        const auto cause = std::make_exception_ptr(el::err::Exception{"The source stream failed."_el});
        const auto error = ConfError{
            ConfErrorContext{ConfErrorCategory::IO, "Reading the Configuration Failed"_el, "Input failed."_el}, cause};
        const auto rendered =
            el::text::StringConverter{el::err::DiagnosticHelper{error}.toDocument().toString()}.toStdString();
        REQUIRE(rendered.find("Caused By") != std::string::npos);
        REQUIRE(rendered.find("The source stream failed.") != std::string::npos);
        REQUIRE(rendered.find("│") == std::string::npos);
    }
};
