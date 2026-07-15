// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathCreateMode.hpp>
#include <erbsland/path/PathReadDataOptions.hpp>
#include <erbsland/text/EncodingError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

using el::mem::ByteBlock;
using el::path::Path;
using el::path::PathContent;
using el::path::PathCreateMode;
using el::path::PathReadDataOptions;
using el::path::PathReadTextOptions;
using el::path::PathWriteDataOptions;
using el::path::PathWriteTextOptions;
using namespace el::text;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(PathContent Path)
class PathContentTest final : public el::UnitTest {
public:
    void testEmptyAndPathAccess() {
        const auto emptyContent = PathContent{};
        REQUIRE(emptyContent.isEmpty());
        REQUIRE(emptyContent.path().isEmpty());
        REQUIRE(&emptyContent.path() == &Path::empty());

        const auto fixture = Fixture{"access"};
        const auto path = pathFromStd(fixture.path() / "report.txt");
        const auto content = path.content();

        REQUIRE_FALSE(content.isEmpty());
        REQUIRE_EQUAL(toStdString(content.path()), toStdString(path));
    }

    void testReadWriteDataAndCreationModes() {
        const auto fixture = Fixture{"data"};
        const auto path = pathFromStd(fixture.path() / "report.bin");
        const auto content = path.content();

        REQUIRE(content.writeData(ByteBlock{std::vector<uint8_t>{1U, 2U, 3U}}).isSuccessful());
        REQUIRE_EQUAL(content.readDataOrThrow().toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));
        REQUIRE(content.writeData(ByteBlock{std::vector<uint8_t>{4U}}).isFailure());

        auto overwrite = PathWriteDataOptions{};
        overwrite.setCreationMode(PathCreateMode::CreateOrOverwrite);
        content.writeDataOrThrow(ByteBlock{std::vector<uint8_t>{4U, 5U}}, overwrite);
        REQUIRE_EQUAL(content.readDataOrThrow().toUInt8Vector(), std::vector<uint8_t>({4U, 5U}));

        auto append = PathWriteDataOptions{};
        append.setCreationMode(PathCreateMode::CreateOrAppend);
        content.writeDataOrThrow(ByteBlock{std::vector<uint8_t>{6U}}, append);
        REQUIRE_EQUAL(content.readDataOrThrow().toUInt8Vector(), std::vector<uint8_t>({4U, 5U, 6U}));
    }

    void testZeroLengthData() {
        const auto fixture = Fixture{"zero"};
        const auto path = pathFromStd(fixture.path() / "empty.bin");
        const auto content = path.content();

        content.writeDataOrThrow(ByteBlock{});

        const auto data = content.readDataOrThrow();
        REQUIRE(data.isEmpty());
    }

    void testCreateParents() {
        const auto fixture = Fixture{"parents"};
        const auto path = pathFromStd(fixture.path() / "one" / "two" / "report.bin");
        const auto content = path.content();

        auto options = PathWriteDataOptions{};
        options.setCreateParents(true);
        content.writeDataOrThrow(ByteBlock{std::vector<uint8_t>{7U}}, options);

        REQUIRE(std::filesystem::exists(fixture.path() / "one" / "two" / "report.bin"));
        REQUIRE_EQUAL(content.readDataOrThrow().toUInt8Vector(), std::vector<uint8_t>({7U}));
    }

    void testTextEncodingAndBom() {
        const auto fixture = Fixture{"text"};
        const auto path = pathFromStd(fixture.path() / "text.txt");
        const auto content = path.content();

        auto writeOptions = PathWriteTextOptions{StringEncoding::Utf16};
        content.writeTextOrThrow("A"_el, writeOptions);

        REQUIRE_EQUAL(content.readDataOrThrow().toUInt8Vector(), std::vector<uint8_t>({0xffU, 0xfeU, 0x41U, 0x00U}));

        auto readOptions = PathReadTextOptions{StringEncoding::Utf16};
        REQUIRE_EQUAL(StringConverter{content.readTextOrThrow(readOptions)}.toStdString(), std::string{"A"});
    }

    void testAppendTextDoesNotWriteMiddleBom() {
        const auto fixture = Fixture{"append-text"};
        const auto path = pathFromStd(fixture.path() / "append.txt");
        const auto content = path.content();

        auto writeOptions = PathWriteTextOptions{StringEncoding::Utf16};
        writeOptions.setBomMode(StringBomMode::Require);
        content.writeTextOrThrow("A"_el, writeOptions);

        auto appendOptions = writeOptions;
        appendOptions.setCreationMode(PathCreateMode::CreateOrAppend);
        content.writeTextOrThrow("B"_el, appendOptions);

        REQUIRE_EQUAL(
            content.readDataOrThrow().toUInt8Vector(),
            std::vector<uint8_t>({0xffU, 0xfeU, 0x41U, 0x00U, 0x42U, 0x00U}));

        const auto appendedNewPath = pathFromStd(fixture.path() / "created-by-append.txt");
        appendedNewPath.content().writeTextOrThrow("C"_el, appendOptions);
        REQUIRE_EQUAL(
            appendedNewPath.content().readDataOrThrow().toUInt8Vector(),
            std::vector<uint8_t>({0xffU, 0xfeU, 0x43U, 0x00U}));
    }

    void testEncodingErrorHandling() {
        const auto fixture = Fixture{"encoding"};
        const auto path = pathFromStd(fixture.path() / "invalid.txt");
        const auto content = path.content();

        content.writeDataOrThrow(ByteBlock{std::vector<uint8_t>{0xffU}});

        auto options = PathReadTextOptions{StringEncoding::Utf8};
        options.setEncodingErrorMode(EncodingErrorMode::Throw);

        REQUIRE_THROWS_AS(el::text::EncodingError, content.readTextOrThrow(options));
        REQUIRE_FALSE(content.readText(options).has_value());
    }

    void testReadLimits() {
        const auto fixture = Fixture{"limits"};
        const auto path = pathFromStd(fixture.path() / "limits.txt");
        const auto content = path.content();
        content.writeTextOrThrow("abc"_el);

        auto dataOptions = PathReadDataOptions{};
        dataOptions.setMaximumByteLength(el::unit::ByteLength{2U});
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, content.readDataOrThrow(dataOptions));
        REQUIRE_FALSE(content.readData(dataOptions).has_value());

        auto options = PathReadTextOptions{el::unit::CpLength{2U}};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, content.readTextOrThrow(options));
        REQUIRE_FALSE(content.readText(options).has_value());
    }

    void testMissingAndDirectoryFailures() {
        const auto fixture = Fixture{"failures"};
        const auto missing = pathFromStd(fixture.path() / "missing.txt").content();
        REQUIRE_FALSE(missing.readData().has_value());

        const auto directory = pathFromStd(fixture.path()).content();
        REQUIRE_FALSE(directory.readData().has_value());

        const auto empty = PathContent{};
        REQUIRE_FALSE(empty.readData().has_value());
        REQUIRE(empty.writeData(ByteBlock{}).isFailure());
    }

private:
    class Fixture final {
    public:
        explicit Fixture(const std::string &name) : _path{createPath(name)} {
            std::filesystem::remove_all(_path);
            std::filesystem::create_directories(_path);
        }

        ~Fixture() { std::filesystem::remove_all(_path); }

        // defaults
        Fixture(const Fixture &) = delete;
        Fixture(Fixture &&) = delete;
        auto operator=(const Fixture &) -> Fixture & = delete;
        auto operator=(Fixture &&) -> Fixture & = delete;

    public:
        [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & { return _path; }

    private:
        [[nodiscard]] static auto createPath(const std::string &name) -> std::filesystem::path {
            const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
            return std::filesystem::temp_directory_path() /
                ("erbsland-core-path-content-" + name + "-" + std::to_string(now));
        }

    private:
        std::filesystem::path _path;
    };

    [[nodiscard]] static auto pathFromStd(const std::filesystem::path &path) -> Path { return Path{path}; }
};
