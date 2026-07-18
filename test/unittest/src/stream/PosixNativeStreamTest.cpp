// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/stream/impl/PosixNativeStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <future>
#include <string>

using el::mem::Byte;
using el::stream::StreamError;
using el::stream::StreamPositionOrigin;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteOffset;
using namespace el::text::literals;

TESTED_TARGETS(PosixNativeStream NativeStreamOwnership NativeStandardStream)
class PosixNativeStreamTest final : public el::UnitTest {
    class Pipe final {
    public:
        Pipe() {
            if (::pipe(_fileDescriptors.data()) != 0) {
                throw StreamError{el::stream::StreamErrorContext{
                    "Failed to create the POSIX test pipe."_el, "The POSIX pipe creation call failed."_el}};
            }
        }

        ~Pipe() {
            closeRead();
            closeWrite();
        }

        // defaults
        Pipe(const Pipe &) = delete;
        Pipe(Pipe &&) = delete;
        auto operator=(const Pipe &) -> Pipe & = delete;
        auto operator=(Pipe &&) -> Pipe & = delete;

    public:
        [[nodiscard]] auto readDescriptor() const noexcept -> int { return _fileDescriptors[0]; }
        [[nodiscard]] auto writeDescriptor() const noexcept -> int { return _fileDescriptors[1]; }

        void closeRead() noexcept {
            if (_fileDescriptors[0] >= 0) {
                static_cast<void>(::close(_fileDescriptors[0]));
                _fileDescriptors[0] = -1;
            }
        }

        void closeWrite() noexcept {
            if (_fileDescriptors[1] >= 0) {
                static_cast<void>(::close(_fileDescriptors[1]));
                _fileDescriptors[1] = -1;
            }
        }

    private:
        std::array<int, 2> _fileDescriptors{-1, -1};
    };

public:
    void testWriteBytesToPipe() {
        auto pipe = Pipe{};
        auto stream = el::stream::impl::PosixNativeStream{
            pipe.writeDescriptor(), el::stream::impl::NativeStreamOwnership::Borrowed};
        const auto expected = std::string{"Pipe text\n"};

        stream.writeBytes(std::span<const char>{expected.data(), expected.size()});
        stream.flush();

        auto buffer = std::array<char, 64>{};
        const auto readCount = ::read(pipe.readDescriptor(), buffer.data(), expected.size());
        REQUIRE_EQUAL(readCount, static_cast<ssize_t>(expected.size()));

        const auto actual = std::string{buffer.data(), static_cast<std::size_t>(readCount)};
        REQUIRE_EQUAL(actual, expected);
    }

    void testReadBytesFromPipe() {
        auto pipe = Pipe{};
        auto stream = el::stream::impl::PosixNativeStream{
            pipe.readDescriptor(), el::stream::impl::NativeStreamOwnership::Borrowed};
        REQUIRE_FALSE(stream.supportsPositioning());
        const auto expected = std::string{"Read text"};
        const auto writeCount = ::write(pipe.writeDescriptor(), expected.data(), expected.size());
        REQUIRE_EQUAL(writeCount, static_cast<ssize_t>(expected.size()));

        auto buffer = std::array<Byte, 32>{};
        const auto readCount = stream.read(std::span<Byte>{buffer});
        REQUIRE_EQUAL(readCount, ByteLength::fromSizeT(expected.size()));

        auto actual = std::string{};
        for (auto i = std::size_t{0}; i < readCount.toSizeT(); ++i) {
            actual.push_back(static_cast<char>(buffer[i].toUInt8()));
        }
        REQUIRE_EQUAL(actual, expected);
    }

    void testInvalidDescriptorThrows() {
        REQUIRE_THROWS_AS(
            StreamError, el::stream::impl::PosixNativeStream{-1, el::stream::impl::NativeStreamOwnership::Borrowed});
    }

    void testNativeErrorIncludesPathAndDescription() {
        auto pipe = Pipe{};
        const auto stream = el::stream::impl::PosixNativeStream{
            pipe.readDescriptor(), el::stream::impl::NativeStreamOwnership::Borrowed, "test/input.pipe"_el};
        try {
            static_cast<void>(stream.position());
            REQUIRE(false);
        } catch (const StreamError &error) {
            REQUIRE_EQUAL(error.title(), "Failed to get the native stream position."_el);
            REQUIRE_EQUAL(error.description(), "The POSIX native stream does not support positioning."_el);
            REQUIRE_EQUAL(error.path(), "test/input.pipe"_el);
        }
    }

    void testOwnedDescriptorClose() {
        auto pipe = Pipe{};
        const auto ownedDescriptor = ::dup(pipe.writeDescriptor());
        REQUIRE(ownedDescriptor >= 0);

        auto stream =
            el::stream::impl::PosixNativeStream{ownedDescriptor, el::stream::impl::NativeStreamOwnership::Owned};
        stream.close();
        stream.close();

        const auto text = std::string{"x"};
        REQUIRE_THROWS_AS(StreamError, stream.writeBytes(std::span<const char>{text.data(), text.size()}));
        errno = 0;
        REQUIRE_EQUAL(::close(ownedDescriptor), -1);
        REQUIRE_EQUAL(errno, EBADF);
    }

    void testAbortDefersCloseUntilBlockedReadFinishes() {
        auto pipe = Pipe{};
        const auto ownedDescriptor = ::dup(pipe.readDescriptor());
        REQUIRE(ownedDescriptor >= 0);
        auto stream =
            el::stream::impl::PosixNativeStream{ownedDescriptor, el::stream::impl::NativeStreamOwnership::Owned};
        auto future = std::async(std::launch::async, [&stream]() -> ByteLength {
            auto buffer = std::array<Byte, 8>{};
            return stream.read(buffer);
        });
        REQUIRE_EQUAL(future.wait_for(std::chrono::milliseconds{50}), std::future_status::timeout);

        stream.abort();
        REQUIRE(::fcntl(ownedDescriptor, F_GETFD) >= 0);

        pipe.closeWrite();
        REQUIRE_EQUAL(future.wait_for(std::chrono::milliseconds{500}), std::future_status::ready);
        REQUIRE(future.get().isZero());
        errno = 0;
        REQUIRE_EQUAL(::fcntl(ownedDescriptor, F_GETFD), -1);
        REQUIRE_EQUAL(errno, EBADF);
    }

    void testFileSize() {
        const auto path = createTemporaryPath();
        const auto fileDescriptor = ::open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, static_cast<mode_t>(0600));
        REQUIRE(fileDescriptor >= 0);

        auto stream =
            el::stream::impl::PosixNativeStream{fileDescriptor, el::stream::impl::NativeStreamOwnership::Owned};
        REQUIRE(stream.fileSize().isZero());

        const auto text = std::string{"Size"};
        stream.writeBytes(std::span<const char>{text.data(), text.size()});

        REQUIRE_EQUAL(stream.fileSize(), ByteLength::fromSizeT(text.size()));
        stream.close();
        std::filesystem::remove(path);
    }

    void testFilePositioningAndAppendRestriction() {
        const auto path = createTemporaryPath();
        const auto fileDescriptor = ::open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, static_cast<mode_t>(0600));
        REQUIRE(fileDescriptor >= 0);
        auto stream =
            el::stream::impl::PosixNativeStream{fileDescriptor, el::stream::impl::NativeStreamOwnership::Owned};

        REQUIRE(stream.supportsPositioning());
        REQUIRE_EQUAL(stream.position(), ByteIndex{0U});
        const auto text = std::string{"Size"};
        stream.writeBytes(std::span<const char>{text.data(), text.size()});
        REQUIRE_EQUAL(stream.position(), ByteIndex{4U});
        REQUIRE_EQUAL(stream.setPosition(ByteIndex{1U}), ByteIndex{1U});
        REQUIRE_EQUAL(stream.movePosition(StreamPositionOrigin::End, ByteOffset{-1}), ByteIndex{3U});
        stream.close();

        const auto appendDescriptor = ::open(path.c_str(), O_WRONLY | O_APPEND);
        REQUIRE(appendDescriptor >= 0);
        auto append =
            el::stream::impl::PosixNativeStream{appendDescriptor, el::stream::impl::NativeStreamOwnership::Owned};
        REQUIRE_FALSE(append.supportsPositioning());
        REQUIRE_THROWS_AS(StreamError, append.position());
        append.close();
        std::filesystem::remove(path);
    }

    void testNativeFailureHasFlatContextAndPath() {
        const auto path = createTemporaryPath();
        const auto fileDescriptor = ::open(path.c_str(), O_CREAT | O_TRUNC | O_RDONLY, static_cast<mode_t>(0600));
        REQUIRE(fileDescriptor >= 0);
        const auto pathText = el::text::StringEditor{std::string_view{path.string()}};

        {
            auto stream = el::stream::impl::PosixNativeStream{
                fileDescriptor, el::stream::impl::NativeStreamOwnership::Owned, pathText};
            const auto bytes = std::array<char, 1>{'x'};
            try {
                stream.writeBytes(bytes);
                REQUIRE(false);
            } catch (const StreamError &error) {
                REQUIRE_FALSE(error.hasCause());
                REQUIRE(error.platformContext() != nullptr);
                REQUIRE_EQUAL(error.path(), pathText);
            }
        }
        std::filesystem::remove(path);
    }

private:
    [[nodiscard]] static auto createTemporaryPath() -> std::filesystem::path {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::filesystem::temp_directory_path() / ("erbsland-core-posix-native-stream-" + std::to_string(now));
    }
};
