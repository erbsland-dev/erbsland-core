// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixNativeStream.hpp"

#include "../../err/StreamError.hpp"

#include <unistd.h>

#include <cerrno>
#include <vector>

namespace erbsland::stream::impl {

PosixNativeStream::PosixNativeStream(const int fileDescriptor, const NativeStreamOwnership ownership) :
    _fileDescriptor{fileDescriptor}, _ownership{ownership} {
    if (_fileDescriptor < 0) {
        throw err::StreamError{"POSIX native output stream file descriptor is not available."};
    }
}

PosixNativeStream::~PosixNativeStream() {
    if (_ownership == NativeStreamOwnership::Owned && _fileDescriptor >= 0) {
        static_cast<void>(::close(_fileDescriptor));
    }
}

void PosixNativeStream::writeBytes(const std::span<const char> bytes) {
    if (!isOpen()) {
        throw err::StreamError{"POSIX native stream is closed."};
    }
    auto position = std::size_t{0};
    while (position < bytes.size()) {
        const auto result = ::write(_fileDescriptor, bytes.data() + position, bytes.size() - position);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw err::StreamError{"Writing to POSIX native output stream failed."};
        }
        if (result == 0) {
            throw err::StreamError{"Writing to POSIX native output stream made no progress."};
        }
        position += static_cast<std::size_t>(result);
    }
}

void PosixNativeStream::flush() {
}

auto PosixNativeStream::endianness() const noexcept -> mem::Endianness {
    return ByteInputStream::endianness();
}

void PosixNativeStream::setEndianness(const mem::Endianness endianness) noexcept {
    ByteInputStream::setEndianness(endianness);
    ByteOutputStream::setEndianness(endianness);
}

auto PosixNativeStream::isOpen() const noexcept -> bool {
    return _fileDescriptor >= 0;
}

void PosixNativeStream::close() {
    if (!isOpen()) {
        return;
    }
    const auto fileDescriptor = _fileDescriptor;
    _fileDescriptor = -1;
    if (_ownership == NativeStreamOwnership::Owned) {
        while (::close(fileDescriptor) != 0) {
            if (errno == EINTR) {
                continue;
            }
            throw err::StreamError{"Closing POSIX native stream failed."};
        }
    }
}

auto PosixNativeStream::read(const std::span<mem::Byte> destination) -> unit::ByteLength {
    if (!isOpen()) {
        throw err::StreamError{"POSIX native stream is closed."};
    }
    if (destination.empty()) {
        return unit::ByteLength::zero();
    }
    while (true) {
        const auto result = ::read(_fileDescriptor, destination.data(), destination.size());
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw err::StreamError{"Reading from POSIX native stream failed."};
        }
        return unit::ByteLength::fromSizeT(static_cast<std::size_t>(result));
    }
}

void PosixNativeStream::write(const std::span<const mem::Byte> bytes) {
    auto rawBytes = std::vector<char>{};
    rawBytes.reserve(bytes.size());
    for (const auto byte : bytes) {
        rawBytes.push_back(static_cast<char>(byte.toUInt8()));
    }
    writeBytes(std::span<const char>{rawBytes});
}

auto createNativeStandardOutputStream(const NativeStandardStream stream) -> NativeOutputStreamPtr {
    switch (stream) {
    case NativeStandardStream::Out:
        return std::make_shared<PosixNativeStream>(STDOUT_FILENO, NativeStreamOwnership::Borrowed);
    case NativeStandardStream::Err:
        return std::make_shared<PosixNativeStream>(STDERR_FILENO, NativeStreamOwnership::Borrowed);
    }
    throw err::StreamError{"Unknown POSIX native standard output stream."};
}

}
