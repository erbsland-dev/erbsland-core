// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MaintenanceProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>

TESTED_TARGETS(tool::github_workflows::GitHubWorkflowsTool tool::include_dependencies::IncludeDependenciesTool)
class MaintenanceValidationLightTest final : public UNITTEST_SUBCLASS(MaintenanceProjectFixture) {
public:
    void testInvalidUpdateRetainsOptionContextWithoutLaunchingCommands() {
        prepareWorkflow();
        REQUIRE_NOT_EQUAL(workflow(el::StringList{"--update"_el, "invalid"_el}), 0);
        REQUIRE(_errorText.contains("--update"_el));
        REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
    }

    void testIrrelevantOptionsFailBeforeScanning() {
        prepareDependencies();
        write("src/erbsland/sample/A.hpp"_el, "#include \"missing.hpp\"\n"_el);
        REQUIRE_NOT_EQUAL(dependencies(el::StringList{"summary"_el, "--max-depth"_el, "1"_el}), 0);
        REQUIRE(_errorText.contains("--max-depth"_el));
        REQUIRE_FALSE(_errorText.contains("Unresolved Quoted Include"_el));
        REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
    }
};
