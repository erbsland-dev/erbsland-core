// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProjectFixture.hpp"

#include <erbsland/StringFormat.hpp>
#include <tool/github_workflows/GitHubWorkflowsTool.hpp>
#include <tool/include_dependencies/IncludeDependenciesTool.hpp>

/// Prepare native external-tool fixtures for maintenance command integration tests.
class MaintenanceProjectFixture : public ProjectFixture {
protected: // fixtures
    /// Write workflow settings and one stable remote tag response.
    void prepareWorkflow() {
        const auto git = el::Path::fromNativeOrThrow(el::String{ERBSLAND_CORE_TOOL_FAKE_GIT});
        const auto zizmor = el::Path::fromNativeOrThrow(el::String{ERBSLAND_CORE_TOOL_FAKE_ZIZMOR});
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            el::StringFormat{
                "[rendering]\n[github_workflows]\nworkflow_directory: \".github/workflows\"\n"
                "git_executable: \"{}\"\nzizmor_executable: \"{}\"\nsafe_binary_directories:\n    * \"{}\"\n"_el}
                .build(git.toString(), zizmor.toString(), git.parent().toString()));
        write(
            ".fixture-tags"_el,
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\trefs/tags/v1.0.0\n"
            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\trefs/tags/v2.0.0\n"_el);
        write(
            ".github/workflows/ci.yml"_el,
            "# owner/repo@v1.0.0\nuses: owner/repo@aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"_el);
    }
    /// Write dependency settings and a small valid source inventory.
    void prepareDependencies() {
        const auto git = el::Path::fromNativeOrThrow(el::String{ERBSLAND_CORE_TOOL_FAKE_GIT});
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            el::StringFormat{
                "[rendering]\n[include_dependencies]\npublic_header_map: \"headers.txt\"\n"
                "git_executable: \"{}\"\nsafe_binary_directories:\n    * \"{}\"\n"
                "*[include_dependencies.required_dependencies]\nname: \"definitions\"\ntarget: \"core/Definitions.hpp\"\n"_el}
                .build(git.toString(), git.parent().toString()));
        write("headers.txt"_el, "sample/A.hpp;A.hpp\n"_el);
        write("src/erbsland/core/Definitions.hpp"_el, "#pragma once\n"_el);
        write("src/erbsland/sample/A.hpp"_el, "#include \"../core/Definitions.hpp\"\n#include <vector>\n"_el);
    }
    /// Read one fixture file.
    [[nodiscard]] auto read(const el::String &path) const -> el::String {
        return (_root / path).content().readTextOrThrow();
    }
    /// Run the workflow command through the full application lifecycle.
    [[nodiscard]] auto workflow(const el::StringList &arguments = {}) -> int {
        return runTool<tool::github_workflows::GitHubWorkflowsTool>({}, {}, "github-workflows"_el, arguments);
    }
    /// Run the dependency command through the full application lifecycle.
    [[nodiscard]] auto dependencies(const el::StringList &arguments = {}) -> int {
        return runTool<tool::include_dependencies::IncludeDependenciesTool>(
            {}, {}, "include-dependencies"_el, arguments);
    }
};
