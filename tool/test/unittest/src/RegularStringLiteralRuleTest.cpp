// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/anti_patterns/rules/RegularStringLiteralRule.hpp>
#include <tool/anti_patterns/SourceIndex.hpp>

#include <array>
#include <string>
#include <utility>

using namespace el::text::literals;

TESTED_TARGETS(tool::anti_patterns::rules::RegularStringLiteralRule)
class RegularStringLiteralRuleTest final : public el::UnitTest {
public:
    void testParameterFailureArguments() {
        using Case = std::pair<el::String, bool>;
        const auto cases = std::array{
            Case{R"(throwParameterFailure("Invalid count", "count");)"_el, false},
            Case{R"(throwParameterError("Invalid count", "count");)"_el, true},
            Case{R"(impl::throwParameterFailure(reason, "count");)"_el, false},
            Case{R"(throw err::ParameterFailure{"Invalid count", "count"};)"_el, false},
            Case{R"(throw err::ParameterFailure("Invalid count", "count");)"_el, false},
            Case{R"(throwParameterFailure(makeReason(first, second), "count");)"_el, false},
            Case{R"(throwParameterFailure(Reason{first, second}, "count");)"_el, false},
            Case{R"(throwParameterFailure(reasons[index], "count");)"_el, false},
            Case{R"(throwParameterFailure /* call */ ("Invalid (count), {value}", /* name */ "count");)"_el, false},
            Case{R"(throwParameterFailure("Invalid count", "cou" "nt");)"_el, false},
            Case{R"(throwParameterFailure("Invalid count", R"name(count)name");)"_el, false},
            Case{"throwParameterFailure(\n    \"Invalid count\",\n    \"count\");"_el, false},
            Case{R"(consume(reason, "count");)"_el, true},
            Case{R"(notthrowParameterFailure(reason, "count");)"_el, true},
            Case{R"(throwParameterFailure("Invalid count", normalize("count"));)"_el, true},
            Case{R"(throwParameterFailure("Invalid count", "count", "extra");)"_el, true},
            Case{R"(consume(throwParameterFailure(reason, "count"), "other");)"_el, true},
            Case{R"(throwParameterFailure(reason, name); auto text = "plain";)"_el, true},
            Case{R"(throwParameterFailure("Invalid count", name + "suffix");)"_el, true},
        };
        const auto rule = tool::anti_patterns::rules::RegularStringLiteralRule{};
        for (const auto &[text, shouldReport] : cases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    const auto source = tool::anti_patterns::SourceIndex{el::Path{"src/Example.cpp"_el}, text};
                    auto candidates = el::List<el::ByteRange>{};
                    rule.scan(source, candidates);
                    REQUIRE_EQUAL(!candidates.isEmpty(), shouldReport);
                },
                [&]() -> std::string { return el::StringConverter{text}.toStdString(); });
        }
    }
};
