// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathCreateMode.hpp>
#include <erbsland/path/PathReadTextOptions.hpp>
#include <erbsland/path/PathWriteDataOptions.hpp>
#include <erbsland/path/PathWriteTextOptions.hpp>
#include <erbsland/stream/ByteInputStream.hpp>
#include <erbsland/stream/ByteOutputStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/stream/StreamPositioning.hpp>
#include <erbsland/stream/StreamPositionOrigin.hpp>
#include <erbsland/stream/StreamPositionStatus.hpp>
#include <erbsland/stream/StringBuilderStream.hpp>
#include <erbsland/stream/TextInputStream.hpp>
#include <erbsland/stream/TextOutputStream.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

using el::mem::Byte;
using el::mem::ByteBlock;
using el::path::Path;
using el::path::PathCreateMode;
using el::path::PathReadTextOptions;
using el::path::PathWriteDataOptions;
using el::path::PathWriteTextOptions;
using el::stream::StreamPositionOrigin;
using el::stream::StreamPositionStatus;
using el::text::StringBomMode;
using el::text::StringEncoding;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteOffset;
using namespace el::text::literals;

TESTED_TARGETS(StreamPositioning StreamPositionOrigin StreamPositionStatus)
class StreamPositionTest final : public el::UnitTest {
    class Fixture final {
    public:
        explicit Fixture(const std::string &name) : _path{createPath(name)} {
            std::filesystem::remove_all(_path);
            std::filesystem::create_directories(_path);
        }
        ~Fixture() { std::filesystem::remove_all(_path); }

        // deletions
        Fixture(const Fixture &) = delete;
        Fixture(Fixture &&) = delete;
        auto operator=(const Fixture &) -> Fixture & = delete;
        auto operator=(Fixture &&) -> Fixture & = delete;

    public:
        [[nodiscard]] auto file(const std::string &name) const -> Path { return Path{_path / name}; }

    private:
        [[nodiscard]] static auto createPath(const std::string &name) -> std::filesystem::path {
            const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
            return std::filesystem::temp_directory_path() /
                ("erbsland-core-stream-position-" + name + "-" + std::to_string(now));
        }

    private:
        std::filesystem::path _path;
    };

public:
    void testStatusAndUnsupportedDefaults() {
        REQUIRE(StreamPositionStatus::Success.isSuccess());
        REQUIRE(StreamPositionStatus::Timeout.isTimeout());
        REQUIRE(StreamPositionStatus::Success.isSuccessful());
        REQUIRE(StreamPositionStatus::Timeout.isFailure());

        const auto stream = el::stream::StringBuilderStream::create();
        REQUIRE_FALSE(stream->supportsPositioning());
        REQUIRE_THROWS_AS(el::stream::StreamError, stream->position());
        REQUIRE_THROWS_AS(el::stream::StreamError, stream->setPosition(ByteIndex{}));
    }

    void testByteInputPositioning() {
        const auto fixture = Fixture{"byte-input"};
        const auto path = fixture.file("data.bin");
        path.content().writeDataOrThrow(ByteBlock{std::vector<uint8_t>{1U, 2U, 3U, 4U}});
        const auto stream = path.content().openByteInputStream();

        REQUIRE(stream->supportsPositioning());
        REQUIRE_EQUAL(stream->position(), ByteIndex{0U});
        REQUIRE_EQUAL(stream->read(ByteLength{2U}).data().toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));
        REQUIRE_EQUAL(stream->position(), ByteIndex{2U});
        REQUIRE(stream->movePosition(StreamPositionOrigin::Current, ByteOffset{-1}).isSuccess());
        REQUIRE_EQUAL(stream->position(), ByteIndex{1U});
        REQUIRE_EQUAL(stream->readByte().data(), Byte{2U});
        REQUIRE(stream->movePosition(StreamPositionOrigin::End, ByteOffset{-1}).isSuccess());
        REQUIRE_EQUAL(stream->readByte().data(), Byte{4U});
        REQUIRE(stream->setPosition(ByteIndex{0U}).isSuccess());
        REQUIRE_EQUAL(stream->readByte().data(), Byte{1U});
    }

    void testByteOutputPositioningAndAppendRestriction() {
        const auto fixture = Fixture{"byte-output"};
        const auto path = fixture.file("data.bin");
        auto options = PathWriteDataOptions{};
        options.setCreationMode(PathCreateMode::CreateOrOverwrite);
        const auto stream = path.content().openByteOutputStream(options);

        REQUIRE(stream->supportsPositioning());
        REQUIRE(stream->write(ByteBlock{std::vector<uint8_t>{1U, 2U, 3U, 4U}}).isSuccess());
        REQUIRE_EQUAL(stream->position(), ByteIndex{4U});
        REQUIRE(stream->setPosition(ByteIndex{1U}).isSuccess());
        REQUIRE(stream->write(Byte{9U}).isSuccess());
        REQUIRE(stream->close().isClosed());
        REQUIRE_EQUAL(path.content().readDataOrThrow().toUInt8Vector(), std::vector<uint8_t>({1U, 9U, 3U, 4U}));

        options.setCreationMode(PathCreateMode::CreateOrAppend);
        const auto append = path.content().openByteOutputStream(options);
        REQUIRE_FALSE(append->supportsPositioning());
        REQUIRE_THROWS_AS(el::stream::StreamError, append->position());
        append->abort();
    }

    void testTextInputUsesEncodedBytePositions() {
        const auto fixture = Fixture{"text-input"};
        const auto path = fixture.file("text.txt");
        path.content().writeTextOrThrow("AéB"_el);
        const auto stream = path.content().openTextInputStream();

        REQUIRE(stream->supportsPositioning());
        REQUIRE_EQUAL(stream->readChar().data(), el::text::Char{U'A'});
        REQUIRE_EQUAL(stream->position(), ByteIndex{1U});
        REQUIRE_EQUAL(stream->readChar().data(), el::text::Char{U'é'});
        REQUIRE_EQUAL(stream->position(), ByteIndex{3U});
        REQUIRE(stream->setPosition(ByteIndex{0U}).isSuccess());
        REQUIRE_EQUAL(stream->readChar().data(), el::text::Char{U'A'});
    }

    void testTextBomPositioning() {
        const auto fixture = Fixture{"text-bom"};
        const auto path = fixture.file("text.txt");
        auto writeOptions = PathWriteTextOptions{StringEncoding::Utf16};
        writeOptions.setCreationMode(PathCreateMode::CreateOrOverwrite);
        writeOptions.setBomMode(StringBomMode::Require);
        path.content().writeTextOrThrow("AB"_el, writeOptions);

        auto readOptions = PathReadTextOptions{StringEncoding::Utf16};
        readOptions.setBomMode(StringBomMode::Require);
        const auto input = path.content().openTextInputStream(readOptions);
        try {
            static_cast<void>(input->setPosition(ByteIndex{2U}));
            REQUIRE(false);
        } catch (const el::stream::StreamError &error) {
            REQUIRE_EQUAL(error.title(), "Failed to set the text stream position."_el);
            REQUIRE_EQUAL(
                error.description(),
                "The text stream byte order must be resolved before positioning away from byte zero."_el);
            REQUIRE_EQUAL(error.path(), path.toString());
        }
        REQUIRE_EQUAL(input->readChar().data(), el::text::Char{U'A'});
        REQUIRE_EQUAL(input->position(), ByteIndex{4U});
        REQUIRE(input->setPosition(ByteIndex{2U}).isSuccess());
        REQUIRE_EQUAL(input->readChar().data(), el::text::Char{U'A'});
        input->abort();

        const auto output = path.content().openTextOutputStream(writeOptions);
        REQUIRE(output->write("A"_el).isSuccess());
        REQUIRE_EQUAL(output->position(), ByteIndex{4U});
        REQUIRE(output->setPosition(ByteIndex{0U}).isSuccess());
        REQUIRE(output->write("B"_el).isSuccess());
        REQUIRE(output->close().isClosed());
        REQUIRE_EQUAL(
            path.content().readDataOrThrow().toUInt8Vector(), std::vector<uint8_t>({0xffU, 0xfeU, 0x42U, 0x00U}));
    }
};
