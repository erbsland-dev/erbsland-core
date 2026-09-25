// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>

namespace demo {

/// Clean and select text supplied by a placeholder source.
///
/// The built-in text filters are opt-in and run from left to right. They can trim surrounding whitespace, create a
/// bounded safe representation, select or remove code-point ranges, remove matching text or characters, and replace
/// exact occurrences. They also provide defaults and Unicode or ASCII case conversion. Indexes and lengths count
/// Unicode code points rather than encoded bytes.
void builtInTextTransforms() {
    auto environment = el::system::EnvironmentVariables{};
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_TITLE"_el, "  Κυματική_ενέργεια-debug  "_el);
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_SERIES"_el, "obs-2026-alpha"_el);
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_SUMMARY"_el, "Θερμοκρασία επιφάνειας και αλατότητα"_el);
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_LABEL"_el, ""_el);

    auto parser = el::conf::Parser{};
    parser.addPlaceholderEnvironmentSource();
    parser.addPlaceholderTextFilters();
    const auto document = parser.parseTextOrThrow(
        "[result]\n"
        "title: \"${env:ERBSLAND_DEMO_RESEARCH_TITLE|trim|remove:text=-debug|replace:text=_,rep=-|lower}\"\n"
        "year: \"${env:ERBSLAND_DEMO_RESEARCH_SERIES|slice:start=4,length=4}\"\n"
        "series: \"${env:ERBSLAND_DEMO_RESEARCH_SERIES|remove:side=front,length=9|upper:ascii}\"\n"
        "summary: \"${env:ERBSLAND_DEMO_RESEARCH_SUMMARY|safe:length=20}\"\n"
        "label: \"${env:ERBSLAND_DEMO_RESEARCH_LABEL|default:pending}\"\n"_el);

    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_TITLE"_el);
    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_SERIES"_el);
    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_SUMMARY"_el);
    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_LABEL"_el);

    el::io::printLine("Title: "_el, document->getTextOrThrow("result.title"_el));
    el::io::printLine("Year: "_el, document->getTextOrThrow("result.year"_el));
    el::io::printLine("Series: "_el, document->getTextOrThrow("result.series"_el));
    el::io::printLine("Summary: "_el, document->getTextOrThrow("result.summary"_el));
    el::io::printLine("Label: "_el, document->getTextOrThrow("result.label"_el));
}

/// Escape text, select a result from a condition, and require a value.
///
/// `escape` prepares the current text for a named target syntax. `if` returns one of two parameter values after testing
/// the current text, while `error_if` preserves the text unless its single condition is true.
void builtInTextConditions() {
    auto environment = el::system::EnvironmentVariables{};
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_MARKUP"_el, "<result id=\"7\">έτοιμο</result>"_el);
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_STATE"_el, "reviewed"_el);
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_ID"_el, "R-104"_el);

    auto parser = el::conf::Parser{};
    parser.addPlaceholderEnvironmentSource();
    parser.addPlaceholderTextFilters();
    const auto document = parser.parseTextOrThrow(
        "[publication]\n"
        "markup: \"${env:ERBSLAND_DEMO_RESEARCH_MARKUP|escape:format=html,amount=required}\"\n"
        "audience: \"${env:ERBSLAND_DEMO_RESEARCH_STATE|if:contains=review,then=internal,else=public}\"\n"
        "result_id: \"${env:ERBSLAND_DEMO_RESEARCH_ID|error_if:empty}\"\n"_el);

    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_MARKUP"_el);
    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_STATE"_el);
    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_ID"_el);

    el::io::printLine("Markup: "_el, document->getTextOrThrow("publication.markup"_el));
    el::io::printLine("Audience: "_el, document->getTextOrThrow("publication.audience"_el));
    el::io::printLine("Result ID: "_el, document->getTextOrThrow("publication.result_id"_el));
}

/// Reject an empty required value while the configuration is parsed.
///
/// `required` reports a configuration validation error when the current text is empty. The parser adds the target name
/// and source location, so a missing runtime value appears in the same diagnostic flow as other configuration problems.
void builtInFilterValidation() {
    auto environment = el::system::EnvironmentVariables{};
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_ID"_el, ""_el);

    auto parser = el::conf::Parser{};
    parser.addPlaceholderEnvironmentSource();
    parser.addPlaceholderTextFilters();
    const auto document = parser.parseTextOrThrow(
        "[publication]\n"
        "result_id: \"${env:ERBSLAND_DEMO_RESEARCH_ID|required}\"\n"_el);
    el::io::printLine("Unexpected result ID: "_el, document->getTextOrThrow("publication.result_id"_el));
}

}
