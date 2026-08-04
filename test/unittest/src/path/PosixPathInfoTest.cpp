// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

using el::path::Path;
using el::path::PathInfoPart;
using el::path::PathType;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(PathInfo PosixPathBackend)
class PosixPathInfoTest final : public el::UnitTest {
public:
    void testRegularFileAndDirectoryInfo() {
        auto scope = ApplicationTestScope<>{};
        const auto fixture = createFixtureDirectory("regular");
        const auto file = fixture / "report.txt";
        {
            auto output = std::ofstream{file};
            output << "hello";
        }

        const auto fileInfo =
            pathFromStd(file).info({PathInfoPart::Size, PathInfoPart::Times, PathInfoPart::Owner, PathInfoPart::Group});
        REQUIRE(fileInfo.exists());
        REQUIRE_EQUAL(fileInfo.type(), PathType::RegularFile);
        REQUIRE(fileInfo.isRegularFile());
        REQUIRE_EQUAL(fileInfo.fileSize(), el::unit::ByteLength{5U});
        REQUIRE(fileInfo.lastModified().isValid());
        REQUIRE_FALSE(fileInfo.ownerId().isEmpty());
        REQUIRE_FALSE(fileInfo.groupId().isEmpty());
        REQUIRE_FALSE(fileInfo.ownerName().isEmpty());
        REQUIRE_FALSE(fileInfo.groupName().isEmpty());
        REQUIRE_FALSE(fileInfo.ownerNameOrThrow().isEmpty());
        REQUIRE_FALSE(fileInfo.groupNameOrThrow().isEmpty());
        REQUIRE(fileInfo.accessInfo().currentProcessRights().hasAny());

        const auto directoryInfo = pathFromStd(fixture).info();
        REQUIRE(directoryInfo.exists());
        REQUIRE(directoryInfo.isDirectory());
        REQUIRE_EQUAL(directoryInfo.type(), PathType::Directory);

        std::filesystem::remove_all(fixture);
    }

    void testMissingPathFallbacks() {
        const auto fixture = createFixtureDirectory("missing");
        const auto missing = pathFromStd(fixture / "missing.txt").info();

        REQUIRE_FALSE(missing.exists());
        REQUIRE_EQUAL(missing.type(), PathType::Unknown);
        REQUIRE(missing.resolvedPath().isEmpty());
        REQUIRE_EQUAL(missing.fileSize(), el::unit::ByteLength{});

        std::filesystem::remove_all(fixture);
    }

    void testSymlinkIsNotFollowed() {
        const auto fixture = createFixtureDirectory("symlink");
        const auto target = fixture / "target.txt";
        const auto link = fixture / "link.txt";
        {
            auto output = std::ofstream{target};
            output << "target";
        }
        std::filesystem::create_symlink(target, link);

        const auto info = pathFromStd(link).info();
        REQUIRE(info.exists());
        REQUIRE(info.isSymlink());
        REQUIRE_EQUAL(info.type(), PathType::Symlink);
        const auto resolvedPathName = toStdString(info.resolvedPath().name());
        REQUIRE_EQUAL(resolvedPathName, "link.txt");

        std::filesystem::remove_all(fixture);
    }

private:
    [[nodiscard]] static auto createFixtureDirectory(const std::string &name) -> std::filesystem::path {
        auto path = std::filesystem::temp_directory_path() /
            ("erbsland-core-path-info-" + name + "-" + std::to_string(::getpid()));
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
        return path;
    }

    [[nodiscard]] static auto pathFromStd(const std::filesystem::path &path) -> Path {
        return Path::fromPosix(el::text::String{path.generic_string()});
    }
};
