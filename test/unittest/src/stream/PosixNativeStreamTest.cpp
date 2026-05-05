// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/StreamError.hpp>
#include <erbsland/stream/impl/PosixNativeStream.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <unistd.h>

#include <array>
#include <string>

using el::err::StreamError;
using el::mem::Byte;
using el::unit::ByteLength;

TESTED_TARGETS(PosixNativeStream NativeStreamOwnership NativeStandardStream)
class PosixNativeStreamTest final : public el::UnitTest {
    class Pipe final {
    public:
        Pipe() {
            if (::pipe(_fileDescriptors.data()) != 0) {
                throw StreamError{"Creating a POSIX test pipe failed."};
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
};
