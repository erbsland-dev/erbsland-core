// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CpCount.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/anti_patterns/Report.hpp>

using namespace el::text::literals;

TESTED_TARGETS(tool::anti_patterns::Report)
class AntiPatternsReportTest final : public el::UnitTest {
public:
    void testStableGroupedReport() {
        using tool::anti_patterns::Finding;
        using tool::anti_patterns::Severity;
        auto findings = el::List<Finding>{
            {"missing_api_documentation"_el,
                Severity::Medium,
                el::Path{"z.hpp"_el},
                el::ByteRange{el::ByteIndex{0}, el::ByteIndex{1}},
                el::LineRange{el::LineIndex{8}, el::LineIndex{9}},
                "last"_el},
            {"anonymous_namespace"_el,
                Severity::High,
                el::Path{"b.cpp"_el},
                el::ByteRange{el::ByteIndex{0}, el::ByteIndex{1}},
                el::LineRange{el::LineIndex{2}, el::LineIndex{3}},
                "second"_el},
            {"missing_api_documentation"_el,
                Severity::Medium,
                el::Path{"z.hpp"_el},
                el::ByteRange{el::ByteIndex{0}, el::ByteIndex{1}},
                el::LineRange{el::LineIndex{1}, el::LineIndex{2}},
                "first"_el},
        };
        const auto report = tool::anti_patterns::Report{};
        const auto text = report.render(findings, el::ItemCount{2}, false);
        REQUIRE(text.find("anonymous_namespace"_el) < text.find("missing_api_documentation"_el));
        REQUIRE(text.contains("z.hpp:2"_el));
        REQUIRE(text.contains("1 more found"_el));
        REQUIRE_EQUAL(
            text,
            "HIGH anonymous_namespace b.cpp:3 | second\n"
            "MEDIUM missing_api_documentation z.hpp:2 | first | 1 more found\n"
            "Legend:\n"
            "anonymous_namespace: doc/guidelines/anti_patterns/anonymous_namespace.rst\n"
            "missing_api_documentation: doc/guidelines/anti_patterns/missing_api_documentation.rst\n"
            "Summary: 3 active, 0 suppressed, 2 shown (limit 2).\n"_el);
        REQUIRE_EQUAL(
            report.render(findings, el::ItemCount::zero(), false),
            "Summary: 3 active, 0 suppressed, 0 shown (limit 0).\n"_el);
        auto suppressed = findings.first();
        suppressed.configReason = " \tAccepted.\r\n\0\u00a0 \v\f"_el;
        findings.set(el::ItemIndex::zero(), suppressed);
        const auto limited = report.render(findings, el::ItemCount{1}, true);
        REQUIRE(limited.contains("Summary: 2 active, 1 suppressed, 1 shown (limit 1)."_el));
        REQUIRE(report.render(findings, el::ItemCount{20}, true).contains("accepted: Accepted. \0\u00a0\n"_el));
        const auto prefix = el::String::fromCharacter(U'ä', el::CpCount{78});
        suppressed.configReason = el::join(prefix, " \0more"_el);
        findings.set(el::ItemIndex::zero(), suppressed);
        REQUIRE(
            report.render(findings, el::ItemCount{20}, true).contains(el::join("accepted: "_el, prefix, " …\n"_el)));
    }
};
