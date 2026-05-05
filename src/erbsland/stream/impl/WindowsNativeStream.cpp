// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsNativeStream.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../err/StreamError.hpp"
#include "../../text/StringConverter.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace erbsland::stream::impl {

WindowsNativeStream::WindowsNativeStream(const WindowsNativeHandle handle, const NativeStreamOwnership ownership) :
    _handle{handle}, _ownership{ownership} {
    if (_handle == nullptr || _handle == INVALID_HANDLE_VALUE) {
        throw err::StreamError{"Windows native output stream handle is not available."};
    }

    auto consoleMode = DWORD{};
    _isConsole = GetConsoleMode(static_cast<HANDLE>(_handle), &consoleMode) != 0;
}

WindowsNativeStream::~WindowsNativeStream() {
    if (_ownership == NativeStreamOwnership::Owned && _handle != nullptr && _handle != INVALID_HANDLE_VALUE) {
        static_cast<void>(CloseHandle(static_cast<HANDLE>(_handle)));
    }
}

void WindowsNativeStream::writeBytes(const std::span<const char> bytes) {
    if (!isOpen()) {
        throw err::StreamError{"Windows native stream is closed."};
    }
    auto position = std::size_t{0};
    while (position < bytes.size()) {
        const auto remaining = bytes.size() - position;
        const auto chunkSize = static_cast<DWORD>(std::min<std::size_t>(remaining, std::numeric_limits<DWORD>::max()));
        auto written = DWORD{};
        const auto ok = WriteFile(static_cast<HANDLE>(_handle), bytes.data() + position, chunkSize, &written, nullptr);
        if (ok == 0) {
            throw err::StreamError{"Writing to Windows native output stream failed."};
        }
        if (written == 0U) {
            throw err::StreamError{"Writing to Windows native output stream made no progress."};
        }
        position += written;
    }
}

void WindowsNativeStream::writeText(const text::StringView &text) {
    if (!_isConsole) {
        NativeOutputStream::writeText(text);
        return;
    }
    writeWideText(erbsland::text::StringConverter{text}.toStdWString());
}

void WindowsNativeStream::flush() {
}

auto WindowsNativeStream::endianness() const noexcept -> mem::Endianness {
    return ByteInputStream::endianness();
}

void WindowsNativeStream::setEndianness(const mem::Endianness endianness) noexcept {
    ByteInputStream::setEndianness(endianness);
    ByteOutputStream::setEndianness(endianness);
}

auto WindowsNativeStream::isOpen() const noexcept -> bool {
    return _handle != nullptr && _handle != INVALID_HANDLE_VALUE;
}

void WindowsNativeStream::close() {
    if (!isOpen()) {
        return;
    }
    const auto handle = _handle;
    _handle = nullptr;
    if (_ownership == NativeStreamOwnership::Owned && CloseHandle(static_cast<HANDLE>(handle)) == 0) {
        throw err::StreamError{"Closing Windows native stream failed."};
    }
}

auto WindowsNativeStream::read(const std::span<mem::Byte> destination) -> unit::ByteLength {
    if (!isOpen()) {
        throw err::StreamError{"Windows native stream is closed."};
    }
    if (destination.empty()) {
        return unit::ByteLength::zero();
    }
    const auto chunkSize =
        static_cast<DWORD>(std::min<std::size_t>(destination.size(), std::numeric_limits<DWORD>::max()));
    auto readCount = DWORD{};
    const auto ok = ReadFile(static_cast<HANDLE>(_handle), destination.data(), chunkSize, &readCount, nullptr);
    if (ok == 0) {
        const auto error = GetLastError();
        if (error == ERROR_BROKEN_PIPE || error == ERROR_HANDLE_EOF) {
            return unit::ByteLength::zero();
        }
        throw err::StreamError{"Reading from Windows native stream failed."};
    }
    return unit::ByteLength::fromSizeT(readCount);
}

void WindowsNativeStream::write(const std::span<const mem::Byte> bytes) {
    auto rawBytes = std::vector<char>{};
    rawBytes.reserve(bytes.size());
    for (const auto byte : bytes) {
        rawBytes.push_back(static_cast<char>(byte.toUInt8()));
    }
    writeBytes(std::span<const char>{rawBytes});
}

void WindowsNativeStream::writeWideText(const std::wstring_view text) {
    if (!isOpen()) {
        throw err::StreamError{"Windows native stream is closed."};
    }
    auto position = std::size_t{0};
    while (position < text.size()) {
        const auto remaining = text.size() - position;
        const auto chunkSize = static_cast<DWORD>(std::min<std::size_t>(remaining, std::numeric_limits<DWORD>::max()));
        auto written = DWORD{};
        const auto ok =
            WriteConsoleW(static_cast<HANDLE>(_handle), text.data() + position, chunkSize, &written, nullptr);
        if (ok == 0) {
            throw err::StreamError{"Writing to Windows console output stream failed."};
        }
        if (written == 0U) {
            throw err::StreamError{"Writing to Windows console output stream made no progress."};
        }
        position += written;
    }
}

auto createNativeStandardOutputStream(const NativeStandardStream stream) -> NativeOutputStreamPtr {
    switch (stream) {
    case NativeStandardStream::Out:
        return std::make_shared<WindowsNativeStream>(GetStdHandle(STD_OUTPUT_HANDLE), NativeStreamOwnership::Borrowed);
    case NativeStandardStream::Err:
        return std::make_shared<WindowsNativeStream>(GetStdHandle(STD_ERROR_HANDLE), NativeStreamOwnership::Borrowed);
    }
    throw err::StreamError{"Unknown Windows native standard output stream."};
}

}
