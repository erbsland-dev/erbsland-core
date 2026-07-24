// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/impl/WindowsApi.hpp>
#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/stream/impl/WindowsNativeStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <future>
#include <span>
#include <string>

using el::mem::Byte;
using el::stream::StreamError;
using el::stream::StreamPositionOrigin;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteOffset;
using namespace el::text::literals;

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
                throw StreamError{el::stream::StreamErrorContext{
                    "Failed to create the Windows test pipe."_el, "The Windows pipe creation call failed."_el}};
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
        REQUIRE_FALSE(stream.supportsPositioning());
        const auto expected = std::string{"Read text"};
        auto writeCount = DWORD{};
        REQUIRE(
            WriteFile(pipe.writeHandle(), expected.data(), static_cast<DWORD>(expected.size()), &writeCount, nullptr) !=
            0);
        REQUIRE_EQUAL(writeCount, static_cast<DWORD>(expected.size()));

        auto buffer = std::array<el::mem::Byte, 32>{};
        const auto readCount = stream.read(el::mem::ByteSpan{buffer});
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

    void testNativeErrorIncludesPathAndDescription() {
        auto pipe = Pipe{};
        const auto stream = el::stream::impl::WindowsNativeStream{
            pipe.readHandle(), el::stream::impl::NativeStreamOwnership::Borrowed, "test/input.pipe"_el};
        try {
            static_cast<void>(stream.position());
            REQUIRE(false);
        } catch (const StreamError &error) {
            REQUIRE_EQUAL(error.title(), "Failed to get the native stream position."_el);
            REQUIRE_EQUAL(error.description(), "The Windows native stream does not support positioning."_el);
            REQUIRE_EQUAL(error.path(), "test/input.pipe"_el);
        }
    }

    void testOwnedHandleClose() {
        auto pipe = Pipe{};
        auto ownedHandle = HANDLE{};
        REQUIRE(
            DuplicateHandle(
                GetCurrentProcess(),
                pipe.writeHandle(),
                GetCurrentProcess(),
                &ownedHandle,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS) != 0);

        auto stream =
            el::stream::impl::WindowsNativeStream{ownedHandle, el::stream::impl::NativeStreamOwnership::Owned};
        stream.close();
        stream.close();

        const auto text = std::string{"x"};
        REQUIRE_THROWS_AS(StreamError, stream.writeBytes(std::span<const char>{text.data(), text.size()}));
        REQUIRE(CloseHandle(ownedHandle) == 0);
    }

    void testAbortCancelsBlockedSynchronousRead() {
        auto pipe = Pipe{};
        auto ownedHandle = HANDLE{};
        REQUIRE(
            DuplicateHandle(
                GetCurrentProcess(),
                pipe.readHandle(),
                GetCurrentProcess(),
                &ownedHandle,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS) != 0);
        auto stream =
            el::stream::impl::WindowsNativeStream{ownedHandle, el::stream::impl::NativeStreamOwnership::Owned};
        auto future = std::async(std::launch::async, [&stream]() -> void {
            auto buffer = std::array<el::mem::Byte, 8>{};
            static_cast<void>(stream.read(el::mem::ByteSpan{buffer}));
        });
        REQUIRE_EQUAL(future.wait_for(std::chrono::milliseconds{50}), std::future_status::timeout);

        stream.abort();
        const auto status = future.wait_for(std::chrono::milliseconds{500});
        if (status != std::future_status::ready) {
            pipe.closeWrite();
            future.wait();
        }

        REQUIRE_EQUAL(status, std::future_status::ready);
        REQUIRE_THROWS_AS(StreamError, future.get());
    }

    void testFileSize() {
        const auto path = createTemporaryPath();
        auto handle = CreateFileW(
            path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        REQUIRE(handle != INVALID_HANDLE_VALUE);

        auto stream = el::stream::impl::WindowsNativeStream{handle, el::stream::impl::NativeStreamOwnership::Owned};
        REQUIRE(stream.fileSize().isZero());

        const auto text = std::string{"Size"};
        stream.writeBytes(std::span<const char>{text.data(), text.size()});

        REQUIRE_EQUAL(stream.fileSize(), ByteLength::fromSizeT(text.size()));
        stream.close();
        std::filesystem::remove(path);
    }

    void testFilePositioningAndRestriction() {
        const auto path = createTemporaryPath();
        auto handle = CreateFileW(
            path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        REQUIRE(handle != INVALID_HANDLE_VALUE);
        auto stream = el::stream::impl::WindowsNativeStream{handle, el::stream::impl::NativeStreamOwnership::Owned};

        REQUIRE(stream.supportsPositioning());
        REQUIRE_EQUAL(stream.position(), ByteIndex{0U});
        const auto text = std::string{"Size"};
        stream.writeBytes(std::span<const char>{text.data(), text.size()});
        REQUIRE_EQUAL(stream.position(), ByteIndex{4U});
        REQUIRE_EQUAL(stream.setPosition(ByteIndex{1U}), ByteIndex{1U});
        REQUIRE_EQUAL(stream.movePosition(StreamPositionOrigin::End, ByteOffset{-1}), ByteIndex{3U});
        stream.close();

        auto restrictedHandle =
            CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        REQUIRE(restrictedHandle != INVALID_HANDLE_VALUE);
        auto restricted = el::stream::impl::WindowsNativeStream{
            restrictedHandle, el::stream::impl::NativeStreamOwnership::Owned, {}, false};
        REQUIRE_FALSE(restricted.supportsPositioning());
        REQUIRE_THROWS_AS(StreamError, restricted.position());
        restricted.close();
        std::filesystem::remove(path);
    }

private:
    [[nodiscard]] static auto createTemporaryPath() -> std::filesystem::path {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::filesystem::temp_directory_path() / ("erbsland-core-windows-native-stream-" + std::to_string(now));
    }
};
