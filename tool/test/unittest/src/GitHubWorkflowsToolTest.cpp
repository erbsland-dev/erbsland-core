// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MaintenanceProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/StringTransformations.hpp>

SKIP_BY_DEFAULT()
TAGS(FullRun)
TESTED_TARGETS(tool::github_workflows::GitHubWorkflowsTool)
class GitHubWorkflowsToolTest final : public UNITTEST_SUBCLASS(MaintenanceProjectFixture) {
public:
    void testEmptyWorkflowDirectory() {
        prepareWorkflow();
        (_root / ".github/workflows/ci.yml"_el).operations().removeOrThrow();
        REQUIRE_EQUAL(workflow(), 0);
        REQUIRE(_outputText.contains("0 workflow files"_el));
        REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
    }

    void testReadOnlyAuditAndOneLookupPerRepository() {
        prepareWorkflow();
        write(
            ".github/workflows/other.yaml"_el,
            "# owner/repo/sub@v1.0.0\nuses: owner/repo/sub@aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"_el);
        const auto original = read(".github/workflows/ci.yml"_el);
        REQUIRE_EQUAL(workflow(), 0);
        REQUIRE_EQUAL(read(".github/workflows/ci.yml"_el), original);
        REQUIRE(_outputText.contains("latest v2.0.0"_el));
        REQUIRE_EQUAL(read(".fixture-calls"_el).count("ls-remote"_el), el::ItemCount{1});
    }

    void testAnnotationFailureAndExplicitFix() {
        prepareWorkflow();
        const auto original = "uses: 'owner/repo@aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa' # keep\r\n"_el;
        write(".github/workflows/ci.yml"_el, original);
        REQUIRE_NOT_EQUAL(workflow(), 0);
        REQUIRE(_errorText.contains("Use --fix"_el));
        REQUIRE_EQUAL(read(".github/workflows/ci.yml"_el), original);
        REQUIRE_EQUAL(workflow(el::StringList{"--fix"_el}), 0);
        REQUIRE_EQUAL(read(".github/workflows/ci.yml"_el), el::join("# owner/repo@v1.0.0\r\n"_el, original));
        REQUIRE_EQUAL(workflow(), 0);
    }

    void testExplicitUpdatePinsUnpinnedReferencesAndIsIdempotent() {
        prepareWorkflow();
        write(".github/workflows/ci.yml"_el, "uses: owner/repo@main\n"_el);
        REQUIRE_EQUAL(workflow(el::StringList{"--update"_el, "owner/repo@latest"_el}), 0);
        const auto updated = read(".github/workflows/ci.yml"_el);
        REQUIRE(updated.contains("# owner/repo@v2.0.0"_el));
        REQUIRE(updated.contains("@bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"_el));
        REQUIRE_EQUAL(workflow(el::StringList{"--update"_el, "owner/repo@v2.0.0"_el}), 0);
        REQUIRE_EQUAL(read(".github/workflows/ci.yml"_el), updated);
    }

    void testExplicitVersionUsesTheHighestAnnotationForItsCommit() {
        prepareWorkflow();
        write(
            ".fixture-tags"_el,
            el::join(read(".fixture-tags"_el), "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\trefs/tags/v3.0.0\n"_el));
        REQUIRE_EQUAL(workflow(el::StringList{"--update"_el, "owner/repo@v2.0.0"_el}), 0);
        REQUIRE(read(".github/workflows/ci.yml"_el).contains("# owner/repo@v3.0.0"_el));
        REQUIRE_EQUAL(workflow(), 0);
    }

    void testInvalidRequestsFailBeforeExternalExecution() {
        prepareWorkflow();
        for (
            const auto &arguments : el::List<el::StringList>{
                el::StringList{"--update"_el, "bad"_el},
                el::StringList{"--update"_el, "owner/repo@v1.0.0"_el, "--update"_el, "owner/repo@v2.0.0"_el},
                el::StringList{"--update"_el, "other/repo@latest"_el}}) {
            REQUIRE_NOT_EQUAL(workflow(arguments), 0);
            REQUIRE(_errorText.contains("--update"_el));
            REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
        }
    }

    void testFailurePreventsEveryPreparedEdit() {
        prepareWorkflow();
        write(".github/workflows/ci.yml"_el, "uses: owner/repo@aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"_el);
        write(".github/workflows/other.yml"_el, "uses: other/repo@main\n"_el);
        const auto original = read(".github/workflows/ci.yml"_el);
        REQUIRE_NOT_EQUAL(workflow(el::StringList{"--fix"_el}), 0);
        REQUIRE_EQUAL(read(".github/workflows/ci.yml"_el), original);
        REQUIRE_NOT_EQUAL(workflow(el::StringList{"--update"_el, "owner/repo@v99.0.0"_el}), 0);
        REQUIRE(_errorText.contains("--update"_el));
        REQUIRE_EQUAL(read(".github/workflows/ci.yml"_el), original);
    }

    void testUnknownShaMalformedRemoteAndZizmorFailures() {
        prepareWorkflow();
        write(".fixture-tags"_el, "cccccccccccccccccccccccccccccccccccccccc\trefs/tags/v3.0.0\n"_el);
        REQUIRE_NOT_EQUAL(workflow(), 0);
        REQUIRE(el::text::toAsciiWhitespaceSimplified(_errorText).contains("no stable tag"_el));
        write(".fixture-tags"_el, "malformed\n"_el);
        REQUIRE_NOT_EQUAL(workflow(), 0);
        REQUIRE(_errorText.contains("Invalid Git Tag Response"_el));
        prepareWorkflow();
        write(".fixture-zizmor"_el, "14"_el);
        REQUIRE_EQUAL(workflow(), 0);
        write(".fixture-zizmor"_el, "2"_el);
        REQUIRE_NOT_EQUAL(workflow(), 0);
        REQUIRE(_errorText.contains("Zizmor Failed"_el));
    }

    void testUnavailableGitAndUnsafeDirectoriesHaveConfigurationContext() {
        prepareWorkflow();
        const auto config = read("tool/conf/erbsland-core-tool.elcl"_el);
        const auto git = el::Path::fromNativeOrThrow(el::String{ERBSLAND_CORE_TOOL_FAKE_GIT});
        write("tool/conf/erbsland-core-tool.elcl"_el, config.replacedAll(git.toString(), "nonexistent-git-command"_el));
        REQUIRE_NOT_EQUAL(workflow(), 0);
        REQUIRE(_errorText.contains("git_executable"_el));
        REQUIRE_FALSE((_root / ".fixture-calls"_el).info().exists());
        write("tool/conf/erbsland-core-tool.elcl"_el, config.replacedAll(git.parent().toString(), "relative"_el));
        REQUIRE_NOT_EQUAL(workflow(), 0);
        REQUIRE(_errorText.contains("safe_binary_directories"_el));
    }

    void testUnavailableRemoteAndTruncatedOutputPreventWrites() {
        prepareWorkflow();
        const auto original = read(".github/workflows/ci.yml"_el);
        (_root / ".fixture-tags"_el).operations().removeOrThrow();
        REQUIRE_NOT_EQUAL(workflow(el::StringList{"--fix"_el}), 0);
        REQUIRE(_errorText.contains("Git Tag Lookup Failed"_el));
        REQUIRE(_errorText.contains("ci.yml"_el));
        write(".fixture-truncated"_el, "yes"_el);
        REQUIRE_NOT_EQUAL(workflow(el::StringList{"--fix"_el}), 0);
        REQUIRE(_errorText.contains("Command Output Truncated"_el));
        REQUIRE(_errorText.contains("ci.yml"_el));
        REQUIRE_EQUAL(read(".github/workflows/ci.yml"_el), original);
    }

    void testFixPreservesUtf8BomAndMissingFinalNewline() {
        prepareWorkflow();
        const auto path = _root / ".github/workflows/ci.yml"_el;
        const auto original = "uses: owner/repo@aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_el;
        path.content().writeTextOrThrow(
            original,
            el::PathWriteTextOptions{}
                .setCreationMode(el::PathCreateMode::CreateOrOverwrite)
                .setBomMode(el::StringBomMode::Require));
        REQUIRE_EQUAL(workflow(el::StringList{"--fix"_el}), 0);
        REQUIRE(path.content().readDataOrThrow().startsWith({0xef, 0xbb, 0xbf}));
        REQUIRE_EQUAL(read(".github/workflows/ci.yml"_el), el::join("# owner/repo@v1.0.0\n"_el, original));
        const auto bytes = path.content().readDataOrThrow();
        REQUIRE_EQUAL(workflow(el::StringList{"--fix"_el}), 0);
        REQUIRE_EQUAL(path.content().readDataOrThrow(), bytes);
    }
};
