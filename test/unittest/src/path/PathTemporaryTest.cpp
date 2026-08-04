// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestFixture.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/PathTempDirectoryOptions.hpp>
#include <erbsland/path/PathTempFileOptions.hpp>
#include <erbsland/path/TempDirectory.hpp>
#include <erbsland/stream/TempByteOutputStream.hpp>
#include <erbsland/stream/TempTextOutputStream.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(
    Path PathTempDirectoryOptions PathTempFileOptions TempDirectory TempByteOutputStream TempTextOutputStream)
class PathTemporaryTest final : public el::UnitTest {
public:
    void testSystemTemporaryDirectory() {
        const auto path = el::path::Path::systemTempDirectoryOrThrow();
        REQUIRE(path.isAbsolute());
        REQUIRE(path.info().isDirectory());
    }

    void testTemporaryDirectoryCleanupAndRelease() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"temporary-directory"};
        auto options = el::path::PathTempDirectoryOptions{};
        options.setPrefix("lease-"_el).setSuffix("-data"_el).setRandomLength(el::unit::CpLength{8U});
        auto temporary = fixture.path().operations().createTempDirectoryOrThrow(options);
        const auto temporaryPath = temporary->path();
        REQUIRE(temporaryPath.info().isDirectory());
        REQUIRE(toStdString(temporaryPath.name()).starts_with("lease-"));
        temporary->removeOrThrow();
        REQUIRE(temporary->isEmpty());
        REQUIRE_FALSE(temporaryPath.info().exists());

        temporary = fixture.path().operations().createTempDirectoryOrThrow(options);
        const auto released = temporary->release();
        REQUIRE(released.info().isDirectory());
        released.operations().removeOrThrow();
    }

    void testTemporaryStreamsCleanupAndRelease() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"temporary-stream"};
        auto options = el::path::PathTempFileOptions{};
        options.setPrefix("stream-"_el).setSuffix(".tmp"_el).setRandomLength(el::unit::CpLength{8U});

        auto byteStream = fixture.path().operations().openTempByteOutputStreamOrThrow(options);
        const auto bytePath = byteStream->path();
        REQUIRE(bytePath.info().isRegularFile());
        byteStream->close();
        REQUIRE(byteStream->isEmpty());
        REQUIRE_FALSE(bytePath.info().exists());

        auto textStream = fixture.path().operations().openTempTextOutputStreamOrThrow(options);
        textStream->write("temporary"_el);
        const auto textPath = textStream->release();
        textStream->close();
        REQUIRE(textPath.info().isRegularFile());
        textPath.operations().removeOrThrow();
    }

    void testDestructorCleanup() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"temporary-destructors"};
        auto directoryPath = el::path::Path{};
        {
            const auto temporary = fixture.path().operations().createTempDirectoryOrThrow();
            directoryPath = temporary->path();
            (directoryPath / "file.txt"_el).content().writeTextOrThrow("data"_el);
        }
        REQUIRE_FALSE(directoryPath.info().exists());

        auto filePath = el::path::Path{};
        {
            const auto temporary = fixture.path().operations().openTempTextOutputStreamOrThrow();
            filePath = temporary->path();
            temporary->write("data"_el);
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{1};
        while (filePath.info().exists() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        REQUIRE_FALSE(filePath.info().exists());
    }

    void testValidationAndCollisionExhaustion() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"temporary-collisions"};
        auto invalidOptions = el::path::PathTempDirectoryOptions{};
        invalidOptions.setPrefix("nested/name-"_el);
        REQUIRE_FALSE(fixture.path().operations().createTempDirectory(invalidOptions));

        constexpr auto alphabet = std::string_view{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
        for (const auto character : alphabet) {
            std::filesystem::create_directory(fixture.stdPath() / ("occupied-" + std::string{character}));
        }
        auto collisionOptions = el::path::PathTempDirectoryOptions{};
        collisionOptions.setPrefix("occupied-"_el)
            .setRandomLength(el::unit::CpLength{1U})
            .setMaximumAttempts(el::unit::ItemCount{2U});
        REQUIRE_FALSE(fixture.path().operations().createTempDirectory(collisionOptions));
    }
};
