// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/impl/WindowsApi.hpp>
#include <erbsland/err/StreamError.hpp>
#include <erbsland/stream/impl/WindowsNativeStream.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <span>
#include <string>

using el::err::StreamError;
using el::mem::Byte;
using el::unit::ByteLength;

TESTED_TARGETS(WindowsNativeStream NativeStreamOwnership NativeStandardStream)
class WindowsNativeStreamTest final : public el::UnitTest {
    class Pipe final {
    public:
        Pipe() {
            auto securityAttributes = SECURITY_ATTRIBUTES{};
            securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
            securityAttributes.bInheritHandle = FALSE;
            securityAttributes.lpSecurityDescriptor = nullptr;
            if (CreatePipe(&_readHandle, &_writeHandle, &securityAttributes, 0) == 0) {
                throw StreamError{"Creating a Windows test pipe failed."};
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
        [[nodiscard]] auto readHandle() const noexcept -> HANDLE { return _readHandle; }
        [[nodiscard]] auto writeHandle() const noexcept -> HANDLE { return _writeHandle; }

        void closeRead() noexcept {
            if (_readHandle != nullptr) {
                static_cast<void>(CloseHandle(_readHandle));
                _readHandle = nullptr;
            }
        }

        void closeWrite() noexcept {
            if (_writeHandle != nullptr) {
                static_cast<void>(CloseHandle(_writeHandle));
                _writeHandle = nullptr;
            }
        }

    private:
        HANDLE _readHandle{};
        HANDLE _writeHandle{};
    };

public:
    void testWriteBytesToPipe() {
        auto pipe = Pipe{};
        auto stream = el::stream::impl::WindowsNativeStream{
            pipe.writeHandle(), el::stream::impl::NativeStreamOwnership::Borrowed};
        const auto expected = std::string{"Pipe text\n"};

        stream.writeBytes(std::span<const char>{expected.data(), expected.size()});
        stream.flush();

        auto buffer = std::array<char, 64>{};
        auto readCount = DWORD{};
        REQUIRE(
            ReadFile(pipe.readHandle(), buffer.data(), static_cast<DWORD>(expected.size()), &readCount, nullptr) != 0);
        const auto actual = std::string{buffer.data(), static_cast<std::size_t>(readCount)};

        REQUIRE_EQUAL(readCount, static_cast<DWORD>(expected.size()));
        REQUIRE_EQUAL(actual, expected);
    }

    void testReadBytesFromPipe() {
        auto pipe = Pipe{};
        auto stream =
            el::stream::impl::WindowsNativeStream{pipe.readHandle(), el::stream::impl::NativeStreamOwnership::Borrowed};
        const auto expected = std::string{"Read text"};
        auto writeCount = DWORD{};
        REQUIRE(
            WriteFile(pipe.writeHandle(), expected.data(), static_cast<DWORD>(expected.size()), &writeCount, nullptr) !=
            0);
        REQUIRE_EQUAL(writeCount, static_cast<DWORD>(expected.size()));

        auto buffer = std::array<Byte, 32>{};
        const auto readCount = stream.read(std::span<Byte>{buffer});
        REQUIRE_EQUAL(readCount, ByteLength::fromSizeT(expected.size()));

        auto actual = std::string{};
        for (auto i = std::size_t{0}; i < readCount.toSizeT(); ++i) {
            actual.push_back(static_cast<char>(buffer[i].toUInt8()));
        }
        REQUIRE_EQUAL(actual, expected);
    }

    void testInvalidHandleThrows() {
        REQUIRE_THROWS_AS(
            StreamError,
            el::stream::impl::WindowsNativeStream{nullptr, el::stream::impl::NativeStreamOwnership::Borrowed});
    }
};
