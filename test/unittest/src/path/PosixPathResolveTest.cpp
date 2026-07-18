// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathResolveMode.hpp>
#include <erbsland/system/PosixErrorContext.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <unistd.h>

#include <exception>
#include <filesystem>
#include <string>
#include <system_error>

using el::path::Path;
using el::path::PathResolveMode;
using el::path::PathResolveOptions;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path PosixPathBackend)
class PosixPathResolveTest final : public el::UnitTest {
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

    void testSymlinkResolutionModes() {
        const auto fixture = createFixture();
        const auto basePath = pathFromStd(fixture);
        std::filesystem::create_directories(fixture / "real");
        if (::symlink("real", (fixture / "link").c_str()) != 0) {
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

    void testPhysicalResolvePreservesPlatformContext() {
        const auto fixture = createFixture();
        const auto missingPath = pathFromStd(fixture / "missing" / "file.txt");

        try {
            static_cast<void>(missingPath.resolveOrThrow(PathResolveMode::Physical));
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            REQUIRE_FALSE(error.hasCause());
            REQUIRE_EQUAL(toStdString(error.sourcePath()), toStdString(missingPath));
            const auto context =
                std::dynamic_pointer_cast<const el::system::PosixErrorContext>(error.platformContext());
            REQUIRE(context != nullptr);
            REQUIRE(context->errorCode() != 0);
        }

        std::filesystem::remove_all(fixture);
    }

private:
    [[nodiscard]] static auto createFixture() -> std::filesystem::path {
        auto fixture = std::filesystem::temp_directory_path() / "erbsland-core-path-resolve";
        fixture /= std::to_string(::getpid());
        std::filesystem::remove_all(fixture);
        std::filesystem::create_directories(fixture);
        return fixture;
    }

    [[nodiscard]] static auto pathFromStd(const std::filesystem::path &path) -> Path {
        return Path::fromPosix(el::text::StringEditor{path.generic_string()});
    }
};
