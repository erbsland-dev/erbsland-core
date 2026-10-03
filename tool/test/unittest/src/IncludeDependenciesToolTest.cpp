// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MaintenanceProjectFixture.hpp"

#include <erbsland/json/JsonValue.hpp>
#include <erbsland/PathInfo.hpp>

#include <filesystem>

SKIP_BY_DEFAULT()
TAGS(FullRun)
TESTED_TARGETS(tool::include_dependencies::IncludeDependenciesTool)
class IncludeDependenciesToolTest final : public UNITTEST_SUBCLASS(MaintenanceProjectFixture) {
public:
    void testModesAndCleanStdoutExport() {
        prepareDependencies();
        REQUIRE_EQUAL(dependencies(), 0);
        REQUIRE(_outputText.contains("Internal nodes: 2"_el));
        REQUIRE_EQUAL(dependencies(el::StringList{"check"_el}), 0);
        REQUIRE_EQUAL(dependencies(el::StringList{"path"_el, "A.hpp"_el, "Definitions.hpp"_el}), 0);
        REQUIRE(_outputText.contains("sample/A.hpp\ncore/Definitions.hpp"_el));
        REQUIRE_EQUAL(dependencies(el::StringList{"tree"_el, "Definitions.hpp"_el, "--reverse"_el}), 0);
        REQUIRE(_outputText.contains("sample/A.hpp"_el));
        REQUIRE_EQUAL(dependencies(el::StringList{"cycles"_el}), 0);
        REQUIRE_EQUAL(dependencies(el::StringList{"export"_el}), 0);
        REQUIRE_EQUAL(el::json::JsonValue::fromStringOrThrow(_outputText).get("schema"_el).getOrThrow<int64_t>(), 1);
        REQUIRE_FALSE((_root / "temporary_data"_el).info().exists());
        REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
    }

    void testInvalidOptionsFailBeforeScanningOrExternalExecution() {
        prepareDependencies();
        write("src/erbsland/sample/A.hpp"_el, "#include \"missing.hpp\"\n"_el);
        for (
            const auto &arguments : el::List<el::StringList>{
                el::StringList{"tree"_el},
                el::StringList{"summary"_el, "--reverse"_el},
                el::StringList{"tree"_el, "A.hpp"_el, "--max-depth"_el, "-1"_el},
                el::StringList{"check"_el, "unknown-rule"_el},
                el::StringList{"export"_el, "--output"_el, "../escape.json"_el},
                el::StringList{"export"_el, "--output"_el, "wrong.dot"_el}}) {
            REQUIRE_NOT_EQUAL(dependencies(arguments), 0);
            REQUIRE_FALSE(_errorText.contains("Unresolved Quoted Include"_el));
            REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
        }
    }

    void testReportRequiresIgnoredAndUntrackedDestination() {
        prepareDependencies();
        const auto arguments = el::StringList{"export"_el, "--output"_el, "graph"_el};
        REQUIRE_NOT_EQUAL(dependencies(arguments), 0);
        REQUIRE_FALSE((_root / "temporary_data"_el).info().exists());
        write(".fixture-ignored"_el, "yes"_el);
        REQUIRE_EQUAL(dependencies(arguments), 0);
        const auto original = read("temporary_data/include_dependencies/graph.json"_el);
        REQUIRE(el::json::JsonValue::fromString(original).has_value());
        write(".fixture-tracked"_el, "yes"_el);
        REQUIRE_NOT_EQUAL(dependencies(arguments), 0);
        REQUIRE_EQUAL(read("temporary_data/include_dependencies/graph.json"_el), original);
    }

    void testConfigurationAndSourceArgumentDiagnostics() {
        prepareDependencies();
        REQUIRE_NOT_EQUAL(dependencies(el::StringList{"tree"_el, "missing.hpp"_el}), 0);
        REQUIRE(_errorText.contains("Invalid Source Argument"_el));
        const auto config = read("tool/conf/erbsland-core-tool.elcl"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            config.replacedAll("core/Definitions.hpp"_el, "core/Missing.hpp"_el));
        REQUIRE_NOT_EQUAL(dependencies(), 0);
        REQUIRE(_errorText.contains("target"_el));
        write("tool/conf/erbsland-core-tool.elcl"_el, config);
        write("headers.txt"_el, "invalid mapping"_el);
        REQUIRE_NOT_EQUAL(dependencies(), 0);
        REQUIRE(_errorText.contains("public_header_map"_el));
    }

    void testRejectsDirectoryAndLinkedReportDestinationsBeforeGit() {
        prepareDependencies();
        write("temporary_data/include_dependencies/directory.json/child"_el, "keep"_el);
        REQUIRE_NOT_EQUAL(dependencies(el::StringList{"export"_el, "--output"_el, "directory.json"_el}), 0);
        REQUIRE(_errorText.contains("--output"_el));
        REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
        write("original.json"_el, "keep"_el);
        auto error = std::error_code{};
        std::filesystem::create_symlink(
            (_root / "original.json"_el).toStdPath(),
            (_root / "temporary_data/include_dependencies/linked.json"_el).toStdPath(),
            error);
        if (error) {
            return; // Windows may lack symbolic-link privileges.
        }
        REQUIRE_NOT_EQUAL(dependencies(el::StringList{"export"_el, "--output"_el, "linked.json"_el}), 0);
        REQUIRE(_errorText.contains("--output"_el));
        REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
        REQUIRE_EQUAL(read("original.json"_el), "keep"_el);
    }
};
