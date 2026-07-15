// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsNativeStream.hpp"

#include "BufferedByteInputStream.hpp"

#include "../StreamError.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../err/ParameterError.hpp"
#include "../../system/WindowsErrorContext.hpp"
#include "../../text/StringConverter.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <utility>

namespace erbsland::stream::impl {

using ErrorContext = system::WindowsErrorContext;
using namespace text::literals;

WindowsNativeStream::Operation::Operation(const WindowsNativeStream &stream) : _stream{stream} {
    const auto lock = std::scoped_lock{_stream._operationMutex};
    _handle = _stream._handle.load();
    if (_handle == nullptr || _handle == INVALID_HANDLE_VALUE) {
        _stream.throwError("Failed to access the native stream."_el, "The Windows native stream is closed."_el);
    }

    auto threadHandle = HANDLE{};
    const auto process = GetCurrentProcess();
    if (DuplicateHandle(process, GetCurrentThread(), process, &threadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS) == 0) {
        _stream.throwErrorFromLastError(
            "Failed to prepare the native stream operation."_el, "Windows could not register the stream operation."_el);
    }
    _threadHandle = threadHandle;
    try {
        _stream._operationThreads.push_back(_threadHandle);
    } catch (...) {
        static_cast<void>(CloseHandle(threadHandle));
        _threadHandle = nullptr;
        throw;
    }
}

WindowsNativeStream::Operation::~Operation() {
    _stream.finishOperation(_threadHandle);
}

auto WindowsNativeStream::createErrorContext() const noexcept -> StreamErrorContext {
    auto context = StreamErrorSource::createErrorContext();
    context.setPath(_path);
    return context;
}

void WindowsNativeStream::throwError(
    const text::StringView title, const text::StringView description, const ErrorContext::ErrorCode errorCode) const {
    auto context = createErrorContext();
    context.setTitle(title).setDescription(description).setPlatformContext(ErrorContext::fromErrorCode(errorCode));
    throw StreamError{std::move(context)};
}

void WindowsNativeStream::throwErrorFromLastError(
    const text::StringView title, const text::StringView description) const {
    auto context = createErrorContext();
    context.setTitle(title).setDescription(description).setPlatformContext(ErrorContext::fromLastError());
    throw StreamError{std::move(context)};
}

WindowsNativeStream::WindowsNativeStream(
    const WindowsNativeHandle handle,
    const NativeStreamOwnership ownership,
    text::StringView path,
    const bool positioningAllowed) :
    _handle{handle}, _ownership{ownership}, _path{std::move(path)} {
    if (_handle.load() == nullptr || _handle.load() == INVALID_HANDLE_VALUE) {
        throwError("Failed to create the native stream."_el, "The Windows native stream handle is not available."_el);
    }

    auto consoleMode = DWORD{};
    _isConsole = GetConsoleMode(static_cast<HANDLE>(_handle.load()), &consoleMode) != 0;
    _supportsPositioning = positioningAllowed && GetFileType(static_cast<HANDLE>(_handle.load())) == FILE_TYPE_DISK;
}

WindowsNativeStream::~WindowsNativeStream() {
    abort();
}

void WindowsNativeStream::writeBytes(const std::span<const char> bytes) {
    const auto operation = Operation{*this};
    auto position = std::size_t{0};
    while (position < bytes.size()) {
        const auto remaining = bytes.size() - position;
        const auto chunkSize = static_cast<DWORD>(std::min<std::size_t>(remaining, std::numeric_limits<DWORD>::max()));
        auto written = DWORD{};
        const auto ok =
            WriteFile(static_cast<HANDLE>(operation.handle()), bytes.data() + position, chunkSize, &written, nullptr);
        if (ok == 0) {
            throwErrorFromLastError(
                "Failed to write to the native stream."_el, "The Windows write operation failed."_el);
        }
        if (written == 0U) {
            throwError(
                "Failed to write to the native stream."_el,
                "The Windows write operation completed without writing any data."_el);
        }
        position += written;
        if (!isOpen()) {
            return;
        }
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

auto WindowsNativeStream::isOpen() const noexcept -> bool {
    const auto handle = _handle.load();
    return handle != nullptr && handle != INVALID_HANDLE_VALUE;
}

auto WindowsNativeStream::supportsPositioning() const noexcept -> bool {
    return _supportsPositioning;
}

auto WindowsNativeStream::position() const -> unit::ByteIndex {
    if (!_supportsPositioning) {
        throwError(
            "Failed to get the native stream position."_el,
            "The Windows native stream does not support positioning."_el);
    }
    const auto operation = Operation{*this};
    const auto distance = LARGE_INTEGER{};
    auto result = LARGE_INTEGER{};
    if (SetFilePointerEx(static_cast<HANDLE>(operation.handle()), distance, &result, FILE_CURRENT) == 0) {
        throwErrorFromLastError(
            "Failed to get the native stream position."_el, "The Windows position lookup failed."_el);
    }
    return unit::ByteIndex{static_cast<unit::ByteIndex::Value>(result.QuadPart)};
}

auto WindowsNativeStream::setPosition(const unit::ByteIndex position) -> unit::ByteIndex {
    if (!_supportsPositioning) {
        throwError(
            "Failed to set the native stream position."_el,
            "The Windows native stream does not support positioning."_el);
    }
    if (position.isNoIndex() ||
        position.toRawValue() > static_cast<unit::ByteIndex::Value>(std::numeric_limits<LONGLONG>::max())) {
        throw err::ParameterError{"Stream position is outside Windows file-offset bounds.", "position"};
    }
    const auto operation = Operation{*this};
    auto distance = LARGE_INTEGER{};
    distance.QuadPart = static_cast<LONGLONG>(position.toRawValue());
    auto result = LARGE_INTEGER{};
    if (SetFilePointerEx(static_cast<HANDLE>(operation.handle()), distance, &result, FILE_BEGIN) == 0) {
        throwErrorFromLastError(
            "Failed to set the native stream position."_el, "The Windows positioning operation failed."_el);
    }
    return unit::ByteIndex{static_cast<unit::ByteIndex::Value>(result.QuadPart)};
}

auto WindowsNativeStream::movePosition(const StreamPositionOrigin origin, const unit::ByteOffset offset)
    -> unit::ByteIndex {
    if (!_supportsPositioning) {
        throwError(
            "Failed to move the native stream position."_el,
            "The Windows native stream does not support positioning."_el);
    }
    auto moveMethod = DWORD{FILE_BEGIN};
    switch (origin) {
    case StreamPositionOrigin::Start:
        moveMethod = FILE_BEGIN;
        break;
    case StreamPositionOrigin::Current:
        moveMethod = FILE_CURRENT;
        break;
    case StreamPositionOrigin::End:
        moveMethod = FILE_END;
        break;
    }
    const auto operation = Operation{*this};
    auto distance = LARGE_INTEGER{};
    distance.QuadPart = offset.toRawValue();
    auto result = LARGE_INTEGER{};
    if (SetFilePointerEx(static_cast<HANDLE>(operation.handle()), distance, &result, moveMethod) == 0) {
        const auto error = GetLastError();
        if (error == ERROR_NEGATIVE_SEEK) {
            throw err::ParameterError{"Resulting stream position is negative.", "offset"};
        }
        throwError(
            "Failed to move the native stream position."_el, "The Windows positioning operation failed."_el, error);
    }
    return unit::ByteIndex{static_cast<unit::ByteIndex::Value>(result.QuadPart)};
}

void WindowsNativeStream::close() {
    auto handle = WindowsNativeHandle{};
    {
        const auto lock = std::scoped_lock{_operationMutex};
        handle = _handle.exchange(nullptr);
        if (_ownership == NativeStreamOwnership::Owned && handle != nullptr && handle != INVALID_HANDLE_VALUE &&
            !_operationThreads.empty()) {
            _deferredCloseHandle = handle;
            handle = nullptr;
        }
    }
    if (_ownership == NativeStreamOwnership::Owned && handle != nullptr && handle != INVALID_HANDLE_VALUE &&
        CloseHandle(static_cast<HANDLE>(handle)) == 0) {
        throwErrorFromLastError("Failed to close the native stream."_el, "The Windows close operation failed."_el);
    }
}

void WindowsNativeStream::abort() noexcept {
    auto handle = WindowsNativeHandle{};
    {
        const auto lock = std::scoped_lock{_operationMutex};
        handle = _handle.exchange(nullptr);
        for (const auto threadHandle : _operationThreads) {
            static_cast<void>(CancelSynchronousIo(static_cast<HANDLE>(threadHandle)));
        }
        if (_ownership == NativeStreamOwnership::Owned && handle != nullptr && handle != INVALID_HANDLE_VALUE &&
            !_operationThreads.empty()) {
            _deferredCloseHandle = handle;
            handle = nullptr;
        }
    }
    if (_ownership == NativeStreamOwnership::Owned && handle != nullptr && handle != INVALID_HANDLE_VALUE) {
        static_cast<void>(CloseHandle(static_cast<HANDLE>(handle)));
    }
}

auto WindowsNativeStream::read(const std::span<mem::Byte> destination) -> unit::ByteLength {
    const auto operation = Operation{*this};
    if (destination.empty()) {
        return unit::ByteLength::zero();
    }
    const auto chunkSize =
        static_cast<DWORD>(std::min<std::size_t>(destination.size(), std::numeric_limits<DWORD>::max()));
    auto readCount = DWORD{};
    const auto ok =
        ReadFile(static_cast<HANDLE>(operation.handle()), destination.data(), chunkSize, &readCount, nullptr);
    if (ok == 0) {
        const auto error = GetLastError();
        if (error == ERROR_BROKEN_PIPE || error == ERROR_HANDLE_EOF) {
            return unit::ByteLength::zero();
        }
        throwError("Failed to read from the native stream."_el, "The Windows read operation failed."_el, error);
    }
    return unit::ByteLength::fromSizeT(readCount);
}

void WindowsNativeStream::write(const std::span<const mem::Byte> bytes) {
    writeBytes(std::span<const char>{reinterpret_cast<const char *>(bytes.data()), bytes.size()});
}

auto WindowsNativeStream::fileSize() const -> unit::ByteLength {
    const auto operation = Operation{*this};
    auto fileSize = LARGE_INTEGER{};
    if (GetFileSizeEx(static_cast<HANDLE>(operation.handle()), &fileSize) == 0) {
        throwErrorFromLastError("Failed to get the native stream size."_el, "The Windows file-size lookup failed."_el);
    }
    if (fileSize.QuadPart <= 0) {
        return unit::ByteLength::zero();
    }
    return unit::ByteLength::fromSizeT(static_cast<std::size_t>(fileSize.QuadPart));
}

void WindowsNativeStream::writeWideText(const std::wstring_view text) {
    const auto operation = Operation{*this};
    auto position = std::size_t{0};
    while (position < text.size()) {
        const auto remaining = text.size() - position;
        const auto chunkSize = static_cast<DWORD>(std::min<std::size_t>(remaining, std::numeric_limits<DWORD>::max()));
        auto written = DWORD{};
        const auto ok = WriteConsoleW(
            static_cast<HANDLE>(operation.handle()), text.data() + position, chunkSize, &written, nullptr);
        if (ok == 0) {
            throwErrorFromLastError(
                "Failed to write to the console."_el, "The Windows console write operation failed."_el);
        }
        if (written == 0U) {
            throwError(
                "Failed to write to the console."_el,
                "The Windows console write operation completed without writing any data."_el);
        }
        position += written;
        if (!isOpen()) {
            return;
        }
    }
}

void WindowsNativeStream::finishOperation(const WindowsNativeHandle threadHandle) const noexcept {
    auto handle = WindowsNativeHandle{};
    {
        const auto lock = std::scoped_lock{_operationMutex};
        const auto position = std::find(_operationThreads.begin(), _operationThreads.end(), threadHandle);
        if (position == _operationThreads.end()) {
            std::terminate();
        }
        _operationThreads.erase(position);
        if (_operationThreads.empty()) {
            handle = std::exchange(_deferredCloseHandle, nullptr);
        }
    }
    static_cast<void>(CloseHandle(static_cast<HANDLE>(threadHandle)));
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
        static_cast<void>(CloseHandle(static_cast<HANDLE>(handle)));
    }
}

auto createNativeStandardOutputStream(const NativeStandardStream stream) -> NativeOutputStreamPtr {
    switch (stream) {
    case NativeStandardStream::Out:
        return std::make_shared<WindowsNativeStream>(GetStdHandle(STD_OUTPUT_HANDLE), NativeStreamOwnership::Borrowed);
    case NativeStandardStream::Err:
        return std::make_shared<WindowsNativeStream>(GetStdHandle(STD_ERROR_HANDLE), NativeStreamOwnership::Borrowed);
    }
    throw StreamError{StreamErrorContext{
        "Failed to create the standard output stream."_el,
        "The requested Windows standard stream is not supported."_el}};
}

auto createNativeStandardInputStream() -> ByteInputStreamPtr {
    return std::make_shared<BufferedByteInputStream>(
        std::make_shared<WindowsNativeStream>(GetStdHandle(STD_INPUT_HANDLE), NativeStreamOwnership::Borrowed));
}

}
