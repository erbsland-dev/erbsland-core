// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestFixture.hpp"

#include <erbsland/path/PathAccessProfile.hpp>
#include <erbsland/path/PathChangeOptions.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathCopyOptions.hpp>
#include <erbsland/path/PathCreateDirectoryOptions.hpp>
#include <erbsland/path/PathCreateFileOptions.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathMoveOptions.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/PathRemoveOptions.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <filesystem>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path PathOperations PathProgress)
class PathOperationsTest final : public el::UnitTest {
public:
    void testCreateFileAndDirectory() {
        const auto fixture = PathTestFixture{"operations-create"};
        auto directoryOptions = el::path::PathCreateDirectoryOptions{};
        directoryOptions.setCreateParents(true);
        fixture.child("one/two").operations().createDirectoryOrThrow(directoryOptions);
        REQUIRE(std::filesystem::is_directory(fixture.stdPath() / "one/two"));

        auto fileOptions = el::path::PathCreateFileOptions{};
        fileOptions.setCreateParents(true);
        fixture.child("three/four.txt").operations().createFileOrThrow(fileOptions);
        REQUIRE(std::filesystem::is_regular_file(fixture.stdPath() / "three/four.txt"));
        REQUIRE(fixture.child("three/four.txt").operations().createFile().isFailure());
    }

    void testCopyMoveAndCollisionModes() {
        const auto fixture = PathTestFixture{"operations-copy"};
        std::filesystem::create_directories(fixture.stdPath() / "source/sub");
        fixture.child("source/a.txt").content().writeTextOrThrow("a"_el);
        fixture.child("source/sub/b.txt").content().writeTextOrThrow("b"_el);

        auto copyOptions = el::path::PathCopyOptions{};
        copyOptions.setRecursive(true);
        copyOptions.setPrescan(true);
        auto lastProgress = el::path::PathProgress{};
        fixture.child("source").operations().copyToOrThrow(
            fixture.child("copy"), copyOptions, [&](const el::path::PathProgress &progress) -> void {
                lastProgress = progress;
            });
        REQUIRE(std::filesystem::is_regular_file(fixture.stdPath() / "copy/sub/b.txt"));
        REQUIRE_EQUAL(lastProgress.total, el::unit::ItemCount{4U});
        REQUIRE_EQUAL(lastProgress.processed, el::unit::ItemCount{4U});

        copyOptions.setCollisionMode(el::path::PathCollisionMode::Skip);
        REQUIRE(fixture.child("source").operations().copyTo(fixture.child("copy"), copyOptions).isSuccessful());
        copyOptions.setCollisionMode(el::path::PathCollisionMode::Overwrite);
        fixture.child("source").operations().copyToOrThrow(fixture.child("copy"), copyOptions);

        auto moveOptions = el::path::PathMoveOptions{};
        moveOptions.setCreateParents(true);
        fixture.child("copy").operations().moveToOrThrow(fixture.child("moved/tree"), moveOptions);
        REQUIRE_FALSE(std::filesystem::exists(fixture.stdPath() / "copy"));
        REQUIRE(std::filesystem::is_regular_file(fixture.stdPath() / "moved/tree/a.txt"));
    }

    void testRecursiveRemoveAndKeepBase() {
        const auto fixture = PathTestFixture{"operations-remove"};
        std::filesystem::create_directories(fixture.stdPath() / "tree/sub");
        fixture.child("tree/sub/file.txt").content().writeTextOrThrow("x"_el);

        auto options = el::path::PathRemoveOptions{};
        options.setRecursive(true);
        options.setKeepBase(true);
        fixture.child("tree").operations().removeOrThrow(options);
        REQUIRE(std::filesystem::is_directory(fixture.stdPath() / "tree"));
        REQUIRE(std::filesystem::is_empty(fixture.stdPath() / "tree"));

        options.setKeepBase(false);
        fixture.child("tree").operations().removeOrThrow(options);
        REQUIRE_FALSE(std::filesystem::exists(fixture.stdPath() / "tree"));
    }

    void testRecursiveAccessProfile() {
        const auto fixture = PathTestFixture{"operations-access"};
        std::filesystem::create_directories(fixture.stdPath() / "tree/sub");
        fixture.child("tree/sub/file.txt").content().writeTextOrThrow("x"_el);

        auto options = el::path::PathChangeOptions{};
        options.setRecursive(true);
        REQUIRE(fixture.child("tree")
                .operations()
                .setAccessProfile(el::path::PathAccessProfile::UserOnly, options)
                .isSuccessful());
        const auto accessInfo = fixture.child("tree/sub/file.txt").info().accessInfo();
        REQUIRE(accessInfo.hasPortableRights() || accessInfo.currentProcessRights().hasAny());
    }

    void testRejectOverlappingAndRootOperations() {
        const auto fixture = PathTestFixture{"operations-safety"};
        std::filesystem::create_directories(fixture.stdPath() / "source/sub");
        auto copyOptions = el::path::PathCopyOptions{};
        copyOptions.setRecursive(true).setCollisionMode(el::path::PathCollisionMode::Overwrite);
        REQUIRE_THROWS_AS(
            el::path::PathError,
            fixture.child("source").operations().copyToOrThrow(fixture.child("source/sub/copy"), copyOptions));
        REQUIRE(std::filesystem::is_directory(fixture.stdPath() / "source/sub"));

        auto removeOptions = el::path::PathRemoveOptions{};
        removeOptions.setRecursive(true);
        const auto root = el::path::Path{fixture.path().root()};
        REQUIRE_THROWS_AS(el::path::PathError, root.operations().removeOrThrow(removeOptions));
    }

    void testIgnoredErrorsStillReturnFailure() {
        const auto fixture = PathTestFixture{"operations-errors"};
        auto removeOptions = el::path::PathRemoveOptions{};
        removeOptions.setIgnoreErrors(true);
        REQUIRE(fixture.child("missing").operations().remove(removeOptions).isFailure());
        REQUIRE_NOTHROW(fixture.child("missing").operations().removeOrThrow(removeOptions));

        auto copyOptions = el::path::PathCopyOptions{};
        copyOptions.setIgnoreErrors(true);
        REQUIRE(fixture.child("missing").operations().copyTo(fixture.child("copy"), copyOptions).isFailure());
        REQUIRE_NOTHROW(fixture.child("missing").operations().copyToOrThrow(fixture.child("copy"), copyOptions));
    }
};
