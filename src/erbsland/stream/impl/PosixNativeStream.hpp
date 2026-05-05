// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeOutputStream.hpp"

#include "../ByteInputStream.hpp"
#include "../ByteOutputStream.hpp"

namespace erbsland::stream::impl {

/// Native byte stream wrapper for POSIX file descriptors.
/// @tested{PosixNativeStreamTest}
class PosixNativeStream final : public NativeOutputStream, public ByteInputStream, public ByteOutputStream {
public:
    /// Create a POSIX native stream wrapper.
    /// @param fileDescriptor The file descriptor to wrap.
    /// @param ownership If the wrapper owns the file descriptor.
    explicit PosixNativeStream(int fileDescriptor, NativeStreamOwnership ownership);

    // defaults
    ~PosixNativeStream() override;
    PosixNativeStream(const PosixNativeStream &) = delete;
    PosixNativeStream(PosixNativeStream &&) = delete;
    auto operator=(const PosixNativeStream &) -> PosixNativeStream & = delete;
    auto operator=(PosixNativeStream &&) -> PosixNativeStream & = delete;

public: // implement NativeOutputStream
    void writeBytes(std::span<const char> bytes) override;
    void flush() override;

public: // implement ByteInputStream / ByteOutputStream
    [[nodiscard]] auto endianness() const noexcept -> mem::Endianness override;
    void setEndianness(mem::Endianness endianness) noexcept override;
    [[nodiscard]] auto isOpen() const noexcept -> bool override;
    void close() override;
    [[nodiscard]] auto read(std::span<mem::Byte> destination) -> unit::ByteLength override;
    void write(std::span<const mem::Byte> bytes) override;

public:
    using ByteInputStream::read;
    using ByteOutputStream::write;

private:
    int _fileDescriptor{-1};
    NativeStreamOwnership _ownership{NativeStreamOwnership::Borrowed};
};

}
