// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include <erbsland/core/impl/WindowsApi.hpp>
#include <erbsland/path/PathResolveMode.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <filesystem>
#include <string>
#include <system_error>

using el::path::Path;
using el::path::PathResolveMode;
using el::path::PathResolveOptions;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path WindowsPathBackend)
class WindowsPathResolveTest final : public el::UnitTest {
public:
    void testCurrentDirectory() {
        const auto currentDirectory = Path::currentDirectory();
        REQUIRE(currentDirectory.isAbsolute());
        REQUIRE(currentDirectory.isValid());
    }

    void testLexicalPhysicalAndWeakResolve() {
        const auto fixture = createFixture();
        const auto basePath = pathFromStd(fixture);
        std::filesystem::create_directories(fixture / "real" / "nested");
        std::filesystem::create_directories(fixture / "base");
        std::filesystem::create_directories(fixture / "real" / "ghost");

        const auto lexical = (basePath / "base/../real/./nested"_el).resolve(PathResolveMode::Lexical);
        REQUIRE_EQUAL(toStdString(lexical), toStdString(pathFromStd(fixture / "real" / "nested")));

        const auto physical = (basePath / "real/nested"_el).resolve(PathResolveMode::Physical);
        REQUIRE_EQUAL(
            toStdString(physical), toStdString(pathFromStd(std::filesystem::canonical(fixture / "real" / "nested"))));

        const auto weak = (basePath / "real/missing/../ghost/file.txt"_el).resolve(PathResolveMode::Weak);
        REQUIRE_EQUAL(
            toStdString(weak),
            toStdString(pathFromStd(std::filesystem::canonical(fixture / "real") / "ghost" / "file.txt")));

        std::filesystem::remove_all(fixture);
    }

    void testSymlinkResolutionModesWhenAvailable() {
        const auto fixture = createFixture();
        const auto basePath = pathFromStd(fixture);
        std::filesystem::create_directories(fixture / "real");
        auto errorCode = std::error_code{};
        std::filesystem::create_directory_symlink(fixture / "real", fixture / "link", errorCode);
        if (errorCode) {
            std::filesystem::remove_all(fixture);
            return;
        }

        const auto physical = (basePath / "link"_el).resolve(PathResolveMode::Physical);
        REQUIRE_EQUAL(toStdString(physical), toStdString(pathFromStd(std::filesystem::canonical(fixture / "real"))));

        const auto noFinalSymlink = (basePath / "link"_el).resolve(PathResolveMode::PhysicalNoFinalSymlink);
        REQUIRE_EQUAL(
            toStdString(noFinalSymlink), toStdString(pathFromStd(std::filesystem::canonical(fixture) / "link")));

        std::filesystem::remove_all(fixture);
    }

private:
    [[nodiscard]] static auto createFixture() -> std::filesystem::path {
        auto fixture = std::filesystem::temp_directory_path() / "erbsland-core-path-resolve";
        fixture /= std::to_string(GetCurrentProcessId());
        std::filesystem::remove_all(fixture);
        std::filesystem::create_directories(fixture);
        return fixture;
    }

    [[nodiscard]] static auto pathFromStd(const std::filesystem::path &path) -> Path {
        return Path::fromWindows(el::text::StringConverter{path.wstring()}.toString());
    }
};
