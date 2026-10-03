// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/json/JsonValue.hpp>
#include <tool/include_dependencies/IncludeGraph.hpp>

TESTED_TARGETS(tool::include_dependencies::IncludeGraph tool::headers::PublicHeaderMap)
class IncludeGraphTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
    /// Schema member containing the graph summary.
    static constexpr auto cSummaryKey = "summary"_el;
    /// Summary member counting regular public headers.
    static constexpr auto cPublicHeadersKey = "public_headers"_el;
    /// Summary member grouping inventory roles.
    static constexpr auto cRolesKey = "roles"_el;

public:
    void testGraphQueriesAndTokenizedIncludes() {
        prepare();
        const auto graph = createGraph();
        REQUIRE_EQUAL(graph.resolve("src/erbsland/sample/A.hpp"_el), "sample/A.hpp"_el);
        REQUIRE_EQUAL(graph.resolve("B.hpp"_el), "sample/B.hpp"_el);
        REQUIRE_EQUAL(
            graph.shortestPath("sample/A.hpp"_el, "core/Definitions.hpp"_el).join("|"_el),
            "sample/A.hpp|sample/B.hpp|core/Definitions.hpp"_el);
        REQUIRE(graph.reachable("core/Definitions.hpp"_el, true).contains("sample/A.hpp"_el));
        REQUIRE_EQUAL(graph.cycles().count(), el::ItemCount{2});
        const auto tree = graph.tree("sample/A.hpp"_el, false, {});
        REQUIRE(tree.contains("sample/B.hpp:7"_el));
        REQUIRE(tree.contains("<vector>:9"_el));
        REQUIRE(graph.tree("sample/A.hpp"_el, false, el::ItemCount{}).contains("[max depth]"_el));
        REQUIRE(graph.tree("sample/C.hpp"_el, false, {}).contains("[cycle]"_el));
        const auto document = el::json::JsonValue::fromStringOrThrow(graph.toJson());
        REQUIRE_EQUAL(document.get("schema"_el).getOrThrow<int64_t>(), 1);
        REQUIRE_EQUAL(document.get(cSummaryKey).get(cPublicHeadersKey).getOrThrow<int64_t>(), 5);
        REQUIRE(graph.toDot().contains("digraph include_dependencies"_el));
    }

    void testMinimalInsertionFrontierAcrossPrivateNodesAndCycles() {
        prepare();
        const auto graph = createGraph();
        const auto analysis = graph.analyze("core/Definitions.hpp"_el, {});
        REQUIRE_EQUAL(analysis.missing.join("|"_el), "sample/C.hpp|sample/D.hpp|sample/Self.hpp"_el);
        REQUIRE_EQUAL(analysis.insertions.join("|"_el), "sample/C.hpp|sample/Self.hpp"_el);
        write("src/erbsland/sample/A.hpp"_el, "#include \"Private.tpp\"\n"_el);
        write("src/erbsland/sample/Private.tpp"_el, "#include \"C.hpp\"\n"_el);
        REQUIRE_EQUAL(
            createGraph().analyze("core/Definitions.hpp"_el, {}).insertions.join("|"_el),
            "sample/C.hpp|sample/Self.hpp"_el);
    }

    void testFailuresAndMappingValidation() {
        prepare();
        write("src/erbsland/other/B.hpp"_el, "\n"_el);
        REQUIRE_THROWS_AS(el::ApplicationError, createGraph().resolve("B.hpp"_el));
        write("src/erbsland/sample/A.hpp"_el, "#include \"Missing.hpp\"\n"_el);
        REQUIRE_THROWS_AS(el::ApplicationError, createGraph());
        auto mapping = tool::headers::PublicHeaderMap{};
        for (
            const auto &text : el::StringList{
                ""_el,
                "../A.hpp;A.hpp\n"_el,
                "sample/B.hpp;B.hpp\nsample/A.hpp;A.hpp\n"_el,
                "sample/A.hpp;A.hpp\nsample/A.hpp;A.hpp\n"_el}) {
            REQUIRE_THROWS_AS(el::ApplicationError, mapping.read(text));
        }
        mapping.read("sample/A.hpp;A.hpp\nsample/A.hpp;sample/A.hpp\nsample/B.hpp;A.hpp\n"_el);
        REQUIRE_EQUAL(mapping.sources().get("A.hpp"_el, {}).count(), el::ItemCount{2});
        REQUIRE_EQUAL(mapping.canonical().get("sample/A.hpp"_el, {}), "A.hpp"_el);
    }

    void testConditionalBranchesDeterministicTiesAndExportEscaping() {
        prepare();
        write(
            "src/erbsland/sample/A.hpp"_el,
            "#if 0\n#include \"D.hpp\"\n#else\n#include \"B.hpp\"\n#endif\n"
            "#include COMPUTED_HEADER\n#include <odd\"quote\\path>\n"_el);
        write("src/erbsland/sample/D.hpp"_el, "#include \"../core/Definitions.hpp\"\n"_el);
        const auto graph = createGraph();
        REQUIRE_EQUAL(
            graph.shortestPath("sample/A.hpp"_el, "core/Definitions.hpp"_el).join("|"_el),
            "sample/A.hpp|sample/B.hpp|core/Definitions.hpp"_el);
        REQUIRE(graph.tree("sample/A.hpp"_el, false, {}).contains("[seen]"_el));
        REQUIRE(graph.toDot().contains("<odd\\\"quote\\\\path>"_el));
        const auto document = el::json::JsonValue::fromStringOrThrow(graph.toJson());
        REQUIRE_EQUAL(document.get(cSummaryKey).get("external_edges"_el).getOrThrow<int64_t>(), 1);
        REQUIRE_EQUAL(graph.toJson(), createGraph().toJson());
    }

    void testForwardHeadersAreExcludedFromPublicChecks() {
        prepare();
        write("src/erbsland/sample/A_fwd.hpp"_el, "#pragma once\n"_el);
        _mapping.read(
            "sample/A.hpp;A.hpp\nsample/A_fwd.hpp;A_fwd.hpp\n"
            "sample/all_sample.hpp;all_sample.hpp\n"_el);
        const auto document = el::json::JsonValue::fromStringOrThrow(createGraph().toJson());
        const auto summary = document.get(cSummaryKey);
        REQUIRE_EQUAL(summary.get(cPublicHeadersKey).getOrThrow<int64_t>(), 1);
        REQUIRE_EQUAL(summary.get(cRolesKey).get("forward"_el).getOrThrow<int64_t>(), 1);
        REQUIRE_EQUAL(summary.get(cRolesKey).get("generated"_el).getOrThrow<int64_t>(), 1);
    }

    void testSourceFailuresRetainPhysicalLine() {
        prepare();
        write("src/erbsland/sample/A.hpp"_el, "// prefix\n\n#include \\\n\"missing.hpp\"\n"_el);
        try {
            const auto graph = createGraph();
            REQUIRE(false);
        } catch (const el::ApplicationError &error) {
            REQUIRE(error.sourcePath().endsWith("sample/A.hpp"_el));
            REQUIRE_EQUAL(error.codeLocation().line(), el::LineIndex{2});
        }
    }

private:
    void prepare() {
        write("src/erbsland/core/Definitions.hpp"_el, "#pragma once\n"_el);
        write(
            "src/erbsland/sample/A.hpp"_el,
            "/*\n#include \"Ignored.hpp\"\n*/\nconst auto text = R\"(\n#include \"Ignored.hpp\"\n)\";\n"
            "# include /* comment */ \\\n\"B.hpp\"\n#include <vector>\n"_el);
        write("src/erbsland/sample/B.hpp"_el, "#include \"../core/Definitions.hpp\"\n"_el);
        write("src/erbsland/sample/C.hpp"_el, "#include \"D.hpp\"\n"_el);
        write("src/erbsland/sample/D.hpp"_el, "#include \"C.hpp\"\n"_el);
        write("src/erbsland/sample/Self.hpp"_el, "#include \"Self.hpp\"\n"_el);
        write("src/erbsland/sample/all_sample.hpp"_el, "#include \"A.hpp\"\n"_el);
        _mapping.read(
            "sample/A.hpp;A.hpp\nsample/B.hpp;B.hpp\nsample/C.hpp;C.hpp\nsample/D.hpp;D.hpp\n"
            "sample/Self.hpp;Self.hpp\nsample/all_sample.hpp;all_sample.hpp\n"_el);
    }
    auto createGraph() const -> tool::include_dependencies::IncludeGraph {
        return tool::include_dependencies::IncludeGraph{files(), _root / "src/erbsland"_el, _mapping};
    }

private:
    tool::headers::PublicHeaderMap _mapping; ///< Fixture publication policy.
};
