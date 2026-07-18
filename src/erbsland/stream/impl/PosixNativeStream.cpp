// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixNativeStream.hpp"

#include "BufferedByteInputStream.hpp"

#include "../StreamError.hpp"

#include "../../err/ParameterError.hpp"
#include "../../system/PosixErrorContext.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <exception>
#include <limits>
#include <utility>

namespace erbsland::stream::impl {

using ErrorContext = system::PosixErrorContext;
using namespace text::literals;

using unit::ByteIndex;
using unit::ByteLength;
using unit::ByteOffset;

PosixNativeStream::Operation::Operation(const PosixNativeStream &stream) : _stream{stream} {
    const auto lock = std::scoped_lock{_stream._operationMutex};
    _fileDescriptor = _stream._fileDescriptor.load();
    if (_fileDescriptor < 0) {
        _stream.throwError("Failed to access the native stream."_el, "The POSIX native stream is closed."_el);
    }
    ++_stream._operationCount;
}

PosixNativeStream::Operation::~Operation() {
    _stream.finishOperation();
}

auto PosixNativeStream::createErrorContext() const noexcept -> StreamErrorContext {
    auto context = StreamErrorSource::createErrorContext();
    context.setPath(_path);
    return context;
}

void PosixNativeStream::throwError(
    text::String title, text::String description, const ErrorContext::ErrorCode errorCode) const {
    auto context = createErrorContext();
    context.setTitle(std::move(title))
        .setDescription(std::move(description))
        .setPlatformContext(ErrorContext::fromErrorCode(errorCode));
    throw StreamError{std::move(context)};
}

void PosixNativeStream::throwErrorFromErrno(text::String title, text::String description) const {
    auto context = createErrorContext();
    context.setTitle(std::move(title))
        .setDescription(std::move(description))
        .setPlatformContext(ErrorContext::fromErrno());
    throw StreamError{std::move(context)};
}

PosixNativeStream::PosixNativeStream(
    const int fileDescriptor, const NativeStreamOwnership ownership, text::String path) :
    _fileDescriptor{fileDescriptor}, _ownership{ownership}, _path{std::move(path)} {
    if (_fileDescriptor < 0) {
        throwError(
            "Failed to create the native stream."_el, "The POSIX native stream file descriptor is not available."_el);
    }
    struct stat info{};
    const auto flags = ::fcntl(_fileDescriptor.load(), F_GETFL);
    _supportsPositioning =
        flags >= 0 && (flags & O_APPEND) == 0 && ::fstat(_fileDescriptor.load(), &info) == 0 && S_ISREG(info.st_mode);
}

PosixNativeStream::~PosixNativeStream() {
    abort();
}

void PosixNativeStream::writeBytes(const std::span<const char> bytes) {
    const auto operation = Operation{*this};
    auto position = std::size_t{0};
    while (position < bytes.size()) {
        const auto result = ::write(operation.fileDescriptor(), bytes.data() + position, bytes.size() - position);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            throwErrorFromErrno("Failed to write to the native stream."_el, "The POSIX write operation failed."_el);
        }
        if (result == 0) {
            throwError(
                "Failed to write to the native stream."_el,
                "The POSIX write operation completed without writing any data."_el);
        }
        position += static_cast<std::size_t>(result);
        if (!isOpen()) {
            return;
        }
    }
}

void PosixNativeStream::flush() {
}

auto PosixNativeStream::isOpen() const noexcept -> bool {
    return _fileDescriptor.load() >= 0;
}

auto PosixNativeStream::supportsPositioning() const noexcept -> bool {
    return _supportsPositioning;
}

auto PosixNativeStream::position() const -> ByteIndex {
    if (!_supportsPositioning) {
        throwError(
            "Failed to get the native stream position."_el, "The POSIX native stream does not support positioning."_el);
    }
    const auto operation = Operation{*this};
    const auto result = ::lseek(operation.fileDescriptor(), 0, SEEK_CUR);
    if (result < 0) {
        throwErrorFromErrno("Failed to get the native stream position."_el, "The POSIX position lookup failed."_el);
    }
    return ByteIndex{static_cast<ByteIndex::Value>(result)};
}

auto PosixNativeStream::setPosition(const ByteIndex position) -> ByteIndex {
    if (!_supportsPositioning) {
        throwError(
            "Failed to set the native stream position."_el, "The POSIX native stream does not support positioning."_el);
    }
    if (position.isNoIndex() ||
        position.toRawValue() > static_cast<ByteIndex::Value>(std::numeric_limits<off_t>::max())) {
        throw err::ParameterError{"Stream position is outside POSIX file-offset bounds.", "position"};
    }
    const auto operation = Operation{*this};
    const auto result = ::lseek(operation.fileDescriptor(), static_cast<off_t>(position.toRawValue()), SEEK_SET);
    if (result < 0) {
        throwErrorFromErrno(
            "Failed to set the native stream position."_el, "The POSIX positioning operation failed."_el);
    }
    return ByteIndex{static_cast<ByteIndex::Value>(result)};
}

auto PosixNativeStream::movePosition(const StreamPositionOrigin origin, const ByteOffset offset) -> ByteIndex {
    if (!_supportsPositioning) {
        throwError(
            "Failed to move the native stream position."_el,
            "The POSIX native stream does not support positioning."_el);
    }
    auto whence = SEEK_SET;
    switch (origin) {
    case StreamPositionOrigin::Start:
        whence = SEEK_SET;
        break;
    case StreamPositionOrigin::Current:
        whence = SEEK_CUR;
        break;
    case StreamPositionOrigin::End:
        whence = SEEK_END;
        break;
    }
    const auto operation = Operation{*this};
    const auto result = ::lseek(operation.fileDescriptor(), static_cast<off_t>(offset.toRawValue()), whence);
    if (result < 0) {
        const auto error = errno;
        if (error == EINVAL) {
            throw err::ParameterError{"Resulting stream position is outside POSIX file-offset bounds.", "offset"};
        }
        throwError(
            "Failed to move the native stream position."_el, "The POSIX positioning operation failed."_el, error);
    }
    return ByteIndex{static_cast<ByteIndex::Value>(result)};
}

void PosixNativeStream::close() {
    auto fileDescriptor = -1;
    {
        const auto lock = std::scoped_lock{_operationMutex};
        fileDescriptor = _fileDescriptor.exchange(-1);
        if (_ownership == NativeStreamOwnership::Owned && fileDescriptor >= 0 && _operationCount > 0U) {
            _deferredCloseDescriptor = fileDescriptor;
            fileDescriptor = -1;
        }
    }
    if (_ownership == NativeStreamOwnership::Owned && fileDescriptor >= 0 && ::close(fileDescriptor) != 0) {
        throwErrorFromErrno("Failed to close the native stream."_el, "The POSIX close operation failed."_el);
    }
}

void PosixNativeStream::abort() noexcept {
    auto fileDescriptor = -1;
    {
        const auto lock = std::scoped_lock{_operationMutex};
        fileDescriptor = _fileDescriptor.exchange(-1);
        if (_ownership == NativeStreamOwnership::Owned && fileDescriptor >= 0 && _operationCount > 0U) {
            _deferredCloseDescriptor = fileDescriptor;
            fileDescriptor = -1;
        }
    }
    if (_ownership == NativeStreamOwnership::Owned && fileDescriptor >= 0) {
        static_cast<void>(::close(fileDescriptor));
    }
}

auto PosixNativeStream::read(const std::span<mem::Byte> destination) -> ByteLength {
    const auto operation = Operation{*this};
    if (destination.empty()) {
        return ByteLength::zero();
    }
    while (true) {
        const auto result = ::read(operation.fileDescriptor(), destination.data(), destination.size());
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            throwErrorFromErrno("Failed to read from the native stream."_el, "The POSIX read operation failed."_el);
        }
        return ByteLength::fromSizeT(static_cast<std::size_t>(result));
    }
}

void PosixNativeStream::write(const std::span<const mem::Byte> bytes) {
    writeBytes(std::span<const char>{reinterpret_cast<const char *>(bytes.data()), bytes.size()});
}

auto PosixNativeStream::fileSize() const -> ByteLength {
    const auto operation = Operation{*this};
    struct stat info{};
    if (::fstat(operation.fileDescriptor(), &info) != 0) {
        throwErrorFromErrno("Failed to get the native stream size."_el, "The POSIX file-size lookup failed."_el);
    }
    if (!S_ISREG(info.st_mode) || info.st_size <= 0) {
        return ByteLength::zero();
    }
    return ByteLength::fromSizeT(static_cast<std::size_t>(info.st_size));
}

void PosixNativeStream::finishOperation() const noexcept {
    auto fileDescriptor = -1;
    {
        const auto lock = std::scoped_lock{_operationMutex};
        if (_operationCount == 0U) {
            std::terminate();
        }
        --_operationCount;
        if (_operationCount == 0U) {
            fileDescriptor = std::exchange(_deferredCloseDescriptor, -1);
        }
    }
    if (fileDescriptor >= 0) {
        static_cast<void>(::close(fileDescriptor));
    }
}

auto createNativeStandardOutputStream(const NativeStandardStream stream) -> NativeOutputStreamPtr {
    switch (stream) {
    case NativeStandardStream::Out:
        return std::make_shared<PosixNativeStream>(STDOUT_FILENO, NativeStreamOwnership::Borrowed);
    case NativeStandardStream::Err:
        return std::make_shared<PosixNativeStream>(STDERR_FILENO, NativeStreamOwnership::Borrowed);
    }
    throw StreamError{StreamErrorContext{
        "Failed to create the standard output stream."_el, "The requested POSIX standard stream is not supported."_el}};
}

auto createNativeStandardInputStream() -> ByteInputStreamPtr {
    return std::make_shared<BufferedByteInputStream>(
        std::make_shared<PosixNativeStream>(STDIN_FILENO, NativeStreamOwnership::Borrowed));
}

}
