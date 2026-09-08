// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestFixture.hpp"

#include <erbsland/path/FileLock.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/system/Subprocess.hpp>
#include <erbsland/system/SubprocessOptions.hpp>
#include <erbsland/system/SubprocessOutputMode.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <utility>

using namespace erbsland::test::pathtest;
using namespace el::text::literals;

TESTED_TARGETS(Path FileLock)
class PathFileLockTest final : public el::UnitTest {
private:
    void requireLockError(const el::path::Path &path, const bool hasPlatformContext) {
        const auto lockPath = el::path::Path{el::text::String::fromJoined({path.toString(), ".lock"_el})};
        try {
            auto lock = path.createLock();
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            REQUIRE_EQUAL(error.title(), "File lock could not be acquired"_el);
            REQUIRE_EQUAL(error.sourcePath(), path.toString());
            REQUIRE_EQUAL(error.targetPath(), hasPlatformContext ? lockPath.toString() : el::text::String{});
            REQUIRE_EQUAL(error.platformContext() != nullptr, hasPlatformContext);
        }
    }

    void requireChildLockState(const el::path::Path &path, const el::text::String &expectedState) {
        auto options = el::system::SubprocessOptions{};
        options.setInheritStandardInput(false)
            .setStandardOutputMode(el::system::SubprocessOutputMode::Capture)
            .setStandardErrorMode(el::system::SubprocessOutputMode::Capture)
            .setEnvironmentVariable("ERBSLAND_FILE_LOCK_TEST_PATH"_el, path.toString())
            .setEnvironmentVariable("ERBSLAND_FILE_LOCK_TEST_STATE"_el, expectedState);
        auto process = el::system::Subprocess::start(
            el::path::Path{el::unittest::fh::unitTestExecutablePath()},
            el::text::StringList{"name:ChildLockAttempt"_el, "--no-color"_el},
            options);
        const auto status = process.wait();
        runWithContext(
            SOURCE_LOCATION(),
            [this, &status]() -> void { REQUIRE(status.isSuccess()); },
            [&process]() -> std::string {
                return "Child standard output:\n" + el::text::StringConverter{process.standardOutput()}.toStdString() +
                    "\nChild standard error:\n" + el::text::StringConverter{process.standardError()}.toStdString();
            });
    }

public:
    void testAcquisitionCreatesPersistentSidecar() {
        const auto fixture = PathTestFixture{"file-lock-sidecar"};
        const auto path = fixture.child("state.elcl");
        const auto lockPath = fixture.child("state.elcl.lock");
        lockPath.content().writeTextOrThrow("existing sidecar content"_el);
        {
            const auto lock = path.createLock();
            REQUIRE(lock.isLocked());
            REQUIRE_EQUAL(lock.path(), path);
            REQUIRE_EQUAL(lock.lockPath(), lockPath);
            REQUIRE(lockPath.info().isRegularFile());
        }
        REQUIRE(lockPath.info().isRegularFile());
        REQUIRE_EQUAL(lockPath.content().readTextOrThrow(), "existing sidecar content"_el);
    }

    void testExclusiveLockAndRelease() {
        const auto fixture = PathTestFixture{"file-lock-exclusive"};
        const auto path = fixture.child("state.elcl");
        {
            const auto lock = path.createLock();
            REQUIRE(lock.isLocked());
            WITH_CONTEXT(requireLockError(path, true));
        }
        const auto lock = path.createLock();
        REQUIRE(lock.isLocked());
    }

    void testMoveConstruction() {
        const auto fixture = PathTestFixture{"file-lock-move-construction"};
        const auto path = fixture.child("state.elcl");
        auto original = path.createLock();
        auto moved = std::move(original);

        REQUIRE_FALSE(original.isLocked());
        REQUIRE(moved.isLocked());
        REQUIRE_EQUAL(moved.path(), path);
        WITH_CONTEXT(requireLockError(path, true));
    }

    void testMoveAssignmentReleasesPreviousLock() {
        const auto fixture = PathTestFixture{"file-lock-move-assignment"};
        const auto firstPath = fixture.child("first.elcl");
        const auto secondPath = fixture.child("second.elcl");
        auto first = firstPath.createLock();
        auto second = secondPath.createLock();

        second = std::move(first);

        REQUIRE_FALSE(first.isLocked());
        REQUIRE(second.isLocked());
        REQUIRE_EQUAL(second.path(), firstPath);
        WITH_CONTEXT(requireLockError(firstPath, true));
        const auto released = secondPath.createLock();
        REQUIRE(released.isLocked());
    }

    void testAcquisitionErrorsHaveContext() {
        WITH_CONTEXT(requireLockError(el::path::Path{}, false));
        const auto fixture = PathTestFixture{"file-lock-error"};
        WITH_CONTEXT(requireLockError(fixture.child("missing/state.elcl"), true));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testInterprocessExclusivity() {
        const auto fixture = PathTestFixture{"file-lock-interprocess"};
        const auto path = fixture.child("state.elcl");
        {
            const auto lock = path.createLock();
            WITH_CONTEXT(requireChildLockState(path, "blocked"_el));
        }
        WITH_CONTEXT(requireChildLockState(path, "available"_el));
    }

    SKIP_BY_DEFAULT()
    void testChildLockAttempt() {
        const auto environment = el::system::EnvironmentVariables{};
        const auto pathText = environment.get("ERBSLAND_FILE_LOCK_TEST_PATH"_el);
        const auto expectedState = environment.get("ERBSLAND_FILE_LOCK_TEST_STATE"_el);
        if (!pathText.has_value() || !expectedState.has_value()) {
            return;
        }
        const auto path = el::path::Path{*pathText};
        if (*expectedState == "blocked"_el) {
            REQUIRE_THROWS_AS(el::path::PathError, path.createLock());
        } else {
            const auto lock = path.createLock();
            REQUIRE(lock.isLocked());
        }
    }
};
