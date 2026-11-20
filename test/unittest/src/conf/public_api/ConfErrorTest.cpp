// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../core/ApplicationTestScope.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/DiagnosticHelper.hpp>
#include <erbsland/Exception.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <exception>
#include <string>
#include <string_view>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(ConfError ConfErrorContext ConfErrorCategory)
class ConfErrorTest final : public el::UnitTest {
public:
    void testValueConstructorsRetainNameAndLocation() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto document = Parser{}.parseTextOrThrow("[main]\nvalue: 42\n"_el);
        const auto value = document->valueOrThrow("main.value"_el);
        for (
            const auto &error :
            {ConfError{value, "Invalid Setting"_el, "Out of range."_el}, ConfError{value, "Out of range."_el}}) {
            REQUIRE_EQUAL(error.namePath(), value->namePath());
            REQUIRE_EQUAL(error.location(), value->location().codeLocation());
            REQUIRE_FALSE(error.title().isEmpty());
        }
    }

    void testContextAccessorsAndOptionalFields() {
        auto applicationScope = ApplicationTestScope<>{};
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
        auto applicationScope = ApplicationTestScope<>{};
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
        auto applicationScope = ApplicationTestScope<>{};
        auto lines = el::text::StringList{};
        lines.append("value: 1"_el);
        auto snippet = el::text::CodeSnippet{std::move(lines), el::unit::LineIndex{4U}, "elcl"_el};
        const auto cause = std::make_exception_ptr(el::err::Exception{"The source stream failed."_el});
        const auto error = ConfError{
            ConfErrorContext{ConfErrorCategory::IO, "Reading the Configuration Failed"_el, "Input failed."_el}, cause};

        const auto unchanged = error.withCodeSnippet(std::nullopt);
        REQUIRE_FALSE(unchanged.context().codeSnippet().has_value());
        REQUIRE_EQUAL(unchanged.cause(), cause);

        const auto enriched = error.withCodeSnippet(snippet);
        REQUIRE(enriched.context().codeSnippet().has_value());
        const auto snippetLanguage = enriched.context().codeSnippet()->language;
        REQUIRE_EQUAL(snippetLanguage, "elcl"_el);
        REQUIRE_EQUAL(enriched.cause(), cause);
        REQUIRE_FALSE(error.context().codeSnippet().has_value());
    }

    void testSourceAwareContextCapturesTheLocationAndSnippet() {
        auto applicationScope = ApplicationTestScope<>{};
        auto source = Source::fromString("zero\none\ntwo\nthree\n"_el);
        const auto location =
            Location{source->identifier(), el::unit::CodeLocation{el::unit::LineIndex{1U}, el::unit::ColumnIndex{1U}}};
        const auto context = ConfErrorContext{ConfErrorCategory::Syntax, "The value is invalid."_el, source, location};

        REQUIRE(context.location().has_value());
        REQUIRE_EQUAL(context.location()->line(), el::unit::LineIndex{1U});
        REQUIRE(context.codeSnippet().has_value());
        REQUIRE_EQUAL(context.codeSnippet()->startLine, el::unit::LineIndex::zero());
        REQUIRE_EQUAL(context.codeSnippet()->lines.count(), el::unit::ItemCount{4U});
    }

    void testExceptionUsesTheTitleAsItsReason() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto error = ConfError{ConfErrorContext{
            ConfErrorCategory::LimitExceeded,
            "A Configuration Limit Was Exceeded"_el,
            "The document contains too many values."_el}};
        REQUIRE_EQUAL(error.reason(), "A Configuration Limit Was Exceeded"_el);
        REQUIRE_EQUAL(error.toString(), "A Configuration Limit Was Exceeded"_el);
        REQUIRE_EQUAL(error.description(), "The document contains too many values."_el);
        requireContains(std::string{error.what()}, "A Configuration Limit Was Exceeded");
    }

    void testPlainTextDiagnosticContainsAllContext() {
        auto applicationScope = ApplicationTestScope<>{};
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

        requireContains(rendered, "Parsing the Configuration Failed");
        requireContains(rendered, "Expected a value, but got an invalid token.");
        requireContains(rendered, "Syntax");
        requireContains(rendered, "main.value");
        requireContains(rendered, "example.elcl");
        requireContains(rendered, "Line:");
        requireContains(rendered, "Column:");
        requireContains(rendered, "2 │ value: ???");
        requireContains(rendered, "▔");
    }

    void testDiagnosticClipsTheMarkerToTheAvailableLine() {
        auto applicationScope = ApplicationTestScope<>{};
        auto lines = el::text::StringList{};
        lines.append("abc"_el);
        auto context = ConfErrorContext{ConfErrorCategory::Syntax, "Parsing Failed"_el, "Invalid input."_el};
        context.setLocation(el::unit::CodeLocation{el::unit::LineIndex::zero(), el::unit::ColumnIndex{99U}})
            .setCodeSnippet(el::text::CodeSnippet{std::move(lines), el::unit::LineIndex::zero(), "elcl"_el});
        const auto rendered =
            el::text::StringConverter{ConfError{std::move(context)}.diagnostic()->toString()}.toStdString();
        requireContains(rendered, "1 │ abc");
        requireContains(rendered, "│   ▔");
    }

    void testGenericDiagnosticHelperAppendsTheCause() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto cause = std::make_exception_ptr(el::err::Exception{"The source stream failed."_el});
        const auto error = ConfError{
            ConfErrorContext{ConfErrorCategory::IO, "Reading the Configuration Failed"_el, "Input failed."_el}, cause};
        const auto rendered =
            el::text::StringConverter{el::err::DiagnosticHelper{error}.toDocument().toString()}.toStdString();
        requireContains(rendered, "Caused By");
        requireContains(rendered, "The source stream failed.");
        requireContains(rendered, "│   The source stream failed.");
    }

private:
    void requireContains(const std::string &text, const std::string_view needle) {
        const auto position = text.find(needle);
        REQUIRE_NOT_EQUAL(position, std::string::npos);
    }
};
