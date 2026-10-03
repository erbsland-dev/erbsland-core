// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/Definitions.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <tool/doc/BuildDocTool.hpp>

#include <filesystem>

using namespace el::text::literals;
TESTED_TARGETS(BuildDocTool)
class BuildDocToolTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
private:
    void prepare() {
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[doc_build]\n*[doc_build.ignored_doxygen_warnings]\nsource_glob: \"erbsland/impl/**\"\nmessage_contains: \"allowed\"\nreason: \"Accepted fixture.\"\n"_el);
        write("fake-sphinx-mode"_el, "success"_el);
        write("_build/keep"_el, "keep"_el);
        write("_doxygen_input/keep"_el, "keep"_el);
#ifdef EL_OS_WINDOWS
        const auto target = _root / ".venv/Scripts/sphinx-build.exe"_el;
#else
        const auto target = _root / ".venv/bin/sphinx-build"_el;
#endif
        el::Path{std::filesystem::path{ERBSLAND_CORE_TOOL_FAKE_SPHINX}}.operations().copyToOrThrow(
            target, el::PathCopyOptions{}.setCreateParents(true));
#ifndef EL_OS_WINDOWS
        std::filesystem::permissions(
            target.toStdPath(), std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
#endif
    }
    auto run(const el::StringList &options = {}) -> int {
        return runTool<tool::doc::BuildDocTool>({}, {}, "doc-build"_el, options);
    }

public:
    void testIncrementalBuildFiltersAndPreservesCaches() {
        prepare();
        REQUIRE_EQUAL(run(), 0);
        REQUIRE((_root / "fake-sphinx-ran"_el).info().exists());
        REQUIRE((_root / "_build/keep"_el).info().exists());
        REQUIRE((_root / "_doxygen_input/keep"_el).info().exists());
        REQUIRE(_outputText.contains("page.rst:12: WARNING: useful diagnostic"_el));
        REQUIRE(_outputText.contains("Suppressed 1 Doxygen warning(s): Accepted fixture."_el));
        REQUIRE_FALSE(_outputText.contains("reading sources..."_el));
        REQUIRE(_outputText.contains("sphinx-build completed successfully."_el));
    }
    void testFullRebuildAndForceAliasRemoveCaches() {
        prepare();
        REQUIRE_EQUAL(run(el::StringList{"--full-rebuild"_el}), 0);
        REQUIRE_FALSE((_root / "_build"_el).info().exists());
        REQUIRE_FALSE((_root / "_doxygen_input"_el).info().exists());
        write("_build/keep"_el, "keep"_el);
        write("_doxygen_input/keep"_el, "keep"_el);
        REQUIRE_EQUAL(run(el::StringList{"--force"_el}), 0);
        REQUIRE_FALSE((_root / "_build"_el).info().exists());
        REQUIRE_FALSE((_root / "_doxygen_input"_el).info().exists());
    }
    void testFailureReplaysUnfilteredDetailsAndExitCode() {
        prepare();
        write("fake-sphinx-mode"_el, "failure"_el);
        REQUIRE(run() != 0);
        REQUIRE(_outputText.contains("Complete sphinx-build output after failure:"_el));
        REQUIRE(_outputText.contains("Traceback detail without a diagnostic prefix"_el));
        REQUIRE(_errorText.contains("sphinx-build failed with exit code 2"_el));
    }
    void testShowSuppressedAndNoFilter() {
        prepare();
        REQUIRE_EQUAL(run(el::StringList{"--show-suppressed"_el}), 0);
        REQUIRE(_outputText.contains("erbsland/impl/Hidden.hpp:9: warning: allowed"_el));
        REQUIRE_EQUAL(run(el::StringList{"--no-filter"_el}), 0);
    }
    void testMissingExecutableAndInvalidConfigurationFailBeforeLaunch() {
        REQUIRE(run() != 0);
        REQUIRE_FALSE((_root / "fake-sphinx-ran"_el).info().exists());
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[doc_build]\nignored_doxygen_warnings: 7\n"_el);
        REQUIRE(run() != 0);
        REQUIRE_FALSE((_root / "fake-sphinx-ran"_el).info().exists());
    }
    void testFullRebuildRejectsDirectorySymlink() {
        prepare();
        (_root / "_build"_el).operations().removeOrThrow(el::PathRemoveOptions{}.setRecursive(true));
        write("retained/keep"_el, "keep"_el);
        auto error = std::error_code{};
        std::filesystem::create_directory_symlink(
            (_root / "retained"_el).toStdPath(), (_root / "_build"_el).toStdPath(), error);
        if (error) {
            return;
        } // Windows may lack symbolic-link privileges.
        REQUIRE(run(el::StringList{"--full-rebuild"_el}) != 0);
        REQUIRE((_root / "retained/keep"_el).info().exists());
    }
};
