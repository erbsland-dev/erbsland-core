// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include "impl/PreCommitProbeTypes.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathInfo.hpp>
#include <tool/frame/ExternalCommand.hpp>
#include <tool/pre_commit/GitState.hpp>
#include <tool/pre_commit/PreCommitTool.hpp>

#include <filesystem>
#include <fstream>

TESTED_TARGETS(GitState CaptureGitStateTask VerifyGitStateTask PreCommitTool NativeGitFile)
SKIP_BY_DEFAULT()
TAGS(FullRun)
class GitStateTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public: // implement UnitTest
    void setUp() override {
        ProjectFixture::setUp();
        test::impl::PreCommitProbe::reset();
        for (const auto &candidate : el::EnvironmentVariables{}.executableCandidatesOrThrow("git"_el)) {
            const auto path = candidate.resolve();
            if (!path.isEmpty() && path.info().isRegularFile() && path.info().isExecutable()) {
                _git = path;
                break;
            }
        }
        REQUIRE_FALSE(_git.isEmpty());
        git(el::StringList{"init"_el});
        git(el::StringList{"config"_el, "user.email"_el, "test@example.invalid"_el});
        git(el::StringList{"config"_el, "user.name"_el, "Pre Commit Test"_el});
        git(el::StringList{"config"_el, "commit.gpgsign"_el, "false"_el});
        write("file.txt"_el, "initial\n"_el);
        git(el::StringList{"add"_el, "."_el});
        git(el::StringList{"commit"_el, "-m"_el, "initial"_el});
    }
    void tearDown() override {
        test::impl::PreCommitProbe::reset();
        ProjectFixture::tearDown();
    }

public:
    void testCleanAndUnchangedDirtyState() {
        auto state = tool::pre_commit::GitState{_root};
        state.capture();
        REQUIRE_NOTHROW(state.verify());
        write("file.txt"_el, "dirty once\n"_el);
        write("untracked.txt"_el, "untracked\n"_el);
        state.capture();
        REQUIRE_NOTHROW(state.verify());
        write("file.txt"_el, "dirty twice\n"_el);
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
    }

    void testIndexAndBinaryChanges() {
        auto state = tool::pre_commit::GitState{_root};
        write("file.txt"_el, "staged\n"_el);
        git(el::StringList{"add"_el, "file.txt"_el});
        state.capture();
        git(el::StringList{"reset"_el, "HEAD"_el, "file.txt"_el});
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
        (_root / "file.txt"_el)
            .content()
            .writeDataOrThrow(
                el::ByteBlock{0, 1, 255},
                el::PathWriteDataOptions{}.setCreationMode(el::PathCreateMode::CreateOrOverwrite));
        state.capture();
        (_root / "file.txt"_el)
            .content()
            .writeDataOrThrow(
                el::ByteBlock{0, 2, 255},
                el::PathWriteDataOptions{}.setCreationMode(el::PathCreateMode::CreateOrOverwrite));
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
        state.capture();
        (_root / "file.txt"_el).operations().removeOrThrow();
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
    }

    void testUntrackedContentAndUnusualNames() {
        // Raw filename construction is intentional: Core text reserves U+FEFF for encoded-data boundaries.
        auto name = el::String{"\uFEFFfirst file with spaces.txt"_el};
#ifndef ERBSLAND_OS_WINDOWS
        name = "\uFEFFfirst file\nwith newline.txt"_el;
#endif
        const auto path = _root.toStdPath() / std::filesystem::path{el::StringConverter{name}.toStdU8String()};
        {
            auto output = std::ofstream{path, std::ios::binary};
            output.write("initial", 7);
            REQUIRE(output.good());
        }
        auto state = tool::pre_commit::GitState{_root};
        state.capture();
        REQUIRE_NOTHROW(state.verify());
        {
            auto output = std::ofstream{path, std::ios::binary};
            output.write("changed", 7);
            REQUIRE(output.good());
        }
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
        state.capture();
        std::filesystem::remove(path);
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
    }

    void testLiteralBackslashFilename() {
#ifndef ERBSLAND_OS_WINDOWS
        const auto name = el::String{"literal\\name.txt"_el};
        const auto path = _root.joinedNative(name);
        path.content().writeTextOrThrow("initial"_el);
        auto state = tool::pre_commit::GitState{_root};
        state.capture();
        REQUIRE_NOTHROW(state.verify());
        path.content().writeTextOrThrow(
            "changed"_el, el::PathWriteTextOptions{}.setCreationMode(el::PathCreateMode::CreateOrOverwrite));
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
#endif
    }

    void testLiteralDanglingSymlinkTargets() {
        const auto link = _root / "link"_el;
        std::filesystem::create_symlink("missing//target", link.toStdPath());
        auto state = tool::pre_commit::GitState{_root};
        state.capture();
        REQUIRE_NOTHROW(state.verify());
        link.operations().removeOrThrow();
#ifdef ERBSLAND_OS_WINDOWS
        // Windows normalizes repeated separators when creating a symlink, so change the target itself.
        std::filesystem::create_symlink("missing/other-target", link.toStdPath());
#else
        std::filesystem::create_symlink("missing/target", link.toStdPath());
#endif
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
    }

    void testTruncatedGitDiffCannotPassVerification() {
        auto state = tool::pre_commit::GitState{_root};
        state.capture();
        write("file.txt"_el, el::String::fromCharacter(U'x', el::CpCount{9 * 1024 * 1024}));
        try {
            state.verify();
            REQUIRE(false);
        } catch (const el::ApplicationError &error) {
            REQUIRE(error.diagnostic()->toString().contains("Command Output Truncated"_el));
        }
    }

    void testFinalVerificationRunsAfterTaskFailure() {
        test::impl::PreCommitProbe::action = [&]() -> void {
            write("file.txt"_el, "changed by failing task\n"_el);
            throw el::ApplicationError{"Fixture Check Failed"_el, "Keep running later tasks."_el};
        };
        REQUIRE_EQUAL(
            runTool<tool::pre_commit::PreCommitTool>(
                {},
                [](TestToolApplication &app) -> void { app.registerPreCommitTask<test::impl::FirstPreCommitProbe>(); },
                "pre-commit"_el),
            1);
        REQUIRE(_outputText.contains("Fixture Check Failed"_el));
        REQUIRE(_outputText.contains("Pre-commit Made Changes"_el));
        REQUIRE(_outputText.contains("1 passed, 2 failed"_el));
    }

    void testMissingBaselineAndUnavailableRepository() {
        auto state = tool::pre_commit::GitState{_root};
        REQUIRE_THROWS_AS(el::ApplicationError, state.verify());
        // Git object files can be read-only on Windows; moving the metadata also makes the repository unavailable.
        (_root / ".git"_el).operations().moveToOrThrow(_root / "detached-git-metadata"_el);
        REQUIRE_EQUAL(
            runTool<tool::pre_commit::PreCommitTool>(
                {},
                [](TestToolApplication &app) -> void { app.registerPreCommitTask<test::impl::FirstPreCommitProbe>(); },
                "pre-commit"_el),
            1);
        REQUIRE(test::impl::PreCommitProbe::events.contains("first_task:execute"_el));
        REQUIRE(_outputText.contains("Git State Comparison Unavailable"_el));
        REQUIRE(_outputText.contains("1 passed, 2 failed"_el));
    }

private:
    void git(const el::StringList &arguments) {
        const auto result = tool::frame::ExternalCommand::runExecutable(_git, arguments, _root, false);
        _failureDiagnostic = result.error;
        REQUIRE(result.status.isSuccess());
    }
    el::Path _git; ///< Resolved Git used only in the isolated fixture.
};
