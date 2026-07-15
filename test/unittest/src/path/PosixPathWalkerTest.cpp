// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestFixture.hpp"

#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathCopyOptions.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/PathWalker.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <filesystem>
#include <string>
#include <vector>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path PathWalker PosixPathBackend)
class PosixPathWalkerTest final : public el::UnitTest {
public:
    void testSymlinkModesAndCycleDetection() {
        const auto fixture = PathTestFixture{"walker-symlinks"};
        std::filesystem::create_directories(fixture.stdPath() / "z-target");
        fixture.child("z-target/file.txt").content().writeTextOrThrow("data"_el);
        std::filesystem::create_directory_symlink("z-target", fixture.stdPath() / "a-link");
        std::filesystem::create_directory_symlink(".", fixture.stdPath() / "cycle");

        auto options = el::path::PathWalkOptions{};
        options.setSymlinkMode(el::path::SymlinkMode::Use).setTypes(el::path::PathType::All);
        auto symlinks = std::vector<std::string>{};
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [&](const el::path::Path &path, const el::path::PathInfo &info) -> el::path::PathWalkStatus {
                        if (info.isSymlink()) {
                            symlinks.push_back(toStdString(path.name()));
                        }
                        return el::path::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE_EQUAL(symlinks, std::vector<std::string>({"a-link", "cycle"}));

        options.setSymlinkMode(el::path::SymlinkMode::Skip);
        symlinks.clear();
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [&](const el::path::Path &, const el::path::PathInfo &info) -> el::path::PathWalkStatus {
                        if (info.isSymlink()) {
                            symlinks.emplace_back("unexpected");
                        }
                        return el::path::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE(symlinks.empty());

        options.setSymlinkMode(el::path::SymlinkMode::Follow);
        auto fileVisits = 0;
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [&](const el::path::Path &, const el::path::PathInfo &info) -> el::path::PathWalkStatus {
                        if (info.isRegularFile()) {
                            ++fileVisits;
                        }
                        return el::path::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE_EQUAL(fileVisits, 1);
    }

    void testCopySymlinkModes() {
        const auto fixture = PathTestFixture{"copy-symlinks"};
        std::filesystem::create_directories(fixture.stdPath() / "source/target");
        fixture.child("source/target/file.txt").content().writeTextOrThrow("data"_el);
        std::filesystem::create_directory_symlink("target", fixture.stdPath() / "source/link");

        auto options = el::path::PathCopyOptions{};
        options.setRecursive(true).setSymlinkMode(el::path::SymlinkMode::Use);
        fixture.child("source").operations().copyToOrThrow(fixture.child("used"), options);
        REQUIRE(std::filesystem::is_symlink(fixture.stdPath() / "used/link"));

        options.setSymlinkMode(el::path::SymlinkMode::Skip);
        fixture.child("source").operations().copyToOrThrow(fixture.child("skipped"), options);
        REQUIRE_FALSE(std::filesystem::exists(fixture.stdPath() / "skipped/link"));

        options.setSymlinkMode(el::path::SymlinkMode::Follow);
        fixture.child("source").operations().copyToOrThrow(fixture.child("followed"), options);
        REQUIRE(std::filesystem::is_regular_file(fixture.stdPath() / "followed/link/file.txt"));
    }
};
