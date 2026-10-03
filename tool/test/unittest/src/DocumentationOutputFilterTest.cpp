// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/PathGlobPattern.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/doc/DocumentationOutputFilter.hpp>

using namespace el::text::literals;
TESTED_TARGETS(DocumentationOutputFilter)
class DocumentationOutputFilterTest final : public el::UnitTest {
public:
    void testRelevantDiagnosticsAndPathShortening() {
        auto filter = tool::doc::DocumentationOutputFilter{el::Path{"/workspace/erbsland-core"_el}};
        REQUIRE_EQUAL(
            filter
                .filterLine(
                    "/workspace/erbsland-core/_doxygen_input/erbsland/Option.hpp:42: warning: unknown command"_el)
                .value(),
            "erbsland/Option.hpp:42: warning: unknown command"_el);
        REQUIRE_EQUAL(
            filter.filterLine("/workspace/erbsland-core/doc/reference/text.rst:12: WARNING: duplicate"_el).value(),
            "reference/text.rst:12: WARNING: duplicate"_el);
        REQUIRE(filter.filterLine("ERROR: summary"_el));
        REQUIRE_FALSE(filter.filterLine("reading sources..."_el));
        REQUIRE_FALSE(filter.filterLine("   continued detail"_el));
        REQUIRE_EQUAL(
            filter.shortenPaths("C:/other/erbsland-core/_doxygen_input/erbsland/Foo.hpp:1: warning: detail"_el),
            "erbsland/Foo.hpp:1: warning: detail"_el);
    }
    void testWindowsExtendedPathsAndSuppressions() {
        auto filter = tool::doc::DocumentationOutputFilter{el::Path::fromWindows("c:/workspace/project"_el)};
        filter.addRule("erbsland/impl/**"_el, "allowed"_el, ""_el, "Internal declaration."_el);
        REQUIRE_FALSE(filter.filterLine(
            "//?/c:/workspace/project/_doxygen_input/erbsland/impl/Hidden.hpp:9: warning: allowed"_el));
        REQUIRE_EQUAL(
            filter.shortenPaths("//?/c:/workspace/project/doc/page.rst:12: WARNING: detail"_el),
            "page.rst:12: WARNING: detail"_el);
        auto network = tool::doc::DocumentationOutputFilter{el::Path::fromWindows("//server/share/project"_el)};
        REQUIRE_EQUAL(
            network.shortenPaths("//?/UNC/server/share/project/doc/page.rst:12: WARNING: detail"_el),
            "page.rst:12: WARNING: detail"_el);
    }
    void testNarrowSuppressionsAndSummary() {
        auto filter = tool::doc::DocumentationOutputFilter{el::Path{"/workspace/erbsland-core"_el}};
        filter.addRule("erbsland/**/impl/**"_el, "is not documented"_el, "Compound"_el, "Internal declaration."_el);
        REQUIRE_FALSE(filter.filterLine(
            "/workspace/erbsland-core/_doxygen_input/erbsland/text/impl/Data.hpp:9: warning: Compound is not documented"_el));
        REQUIRE(filter.filterLine(
            "/workspace/erbsland-core/_doxygen_input/erbsland/text/impl/Data.hpp:10: warning: Member is not documented"_el));
        REQUIRE(filter.filterLine(
            "/workspace/erbsland-core/_doxygen_input/erbsland/text/impl/Data.hpp:11: warning: unknown command"_el));
        REQUIRE_EQUAL(
            filter.summary().get(el::ItemIndex{}), "Suppressed 1 Doxygen warning(s): Internal declaration."_el);
        auto visible = tool::doc::DocumentationOutputFilter{el::Path{"/workspace/erbsland-core"_el}, true};
        visible.addRule("**"_el, ""_el, ""_el, "Everything"_el);
        REQUIRE(visible.filterLine("erbsland/Data.hpp:1: warning: detail"_el));
        REQUIRE(visible.summary().isEmpty());
    }
    void testPathGlobSemantics() {
        auto filter = tool::doc::DocumentationOutputFilter{el::Path{"/workspace"_el}};
        filter.addRule("src/*.[ch]pp"_el, "allowed"_el, ""_el, "Immediate sources."_el);
        REQUIRE_FALSE(filter.filterLine("src/test.cpp:1: warning: allowed"_el));
        REQUIRE(filter.filterLine("src/deep/test.cpp:1: warning: allowed"_el));
        filter.addRule("src/**/hidden[^0-9].hpp"_el, "allowed"_el, ""_el, "Hidden headers."_el);
        REQUIRE_FALSE(filter.filterLine("src/deep/hiddenA.hpp:1: warning: allowed"_el));
        REQUIRE(filter.filterLine("src/deep/hidden1.hpp:1: warning: allowed"_el));
    }
};
