// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeOutputStream.hpp"

#include "../ByteInputStream.hpp"
#include "../ByteOutputStream.hpp"

#include <string_view>

namespace erbsland::stream::impl {

/// A native Windows stream handle.
using WindowsNativeHandle = void *;

/// Native byte stream wrapper for Windows handles.
/// @tested{WindowsNativeStreamTest}
class WindowsNativeStream final : public NativeOutputStream, public ByteInputStream, public ByteOutputStream {
public:
    /// Create a Windows native stream wrapper.
    /// @param handle The handle to wrap.
    /// @param ownership If the wrapper owns the handle.
    explicit WindowsNativeStream(WindowsNativeHandle handle, NativeStreamOwnership ownership);

    // defaults
    ~WindowsNativeStream() override;
    WindowsNativeStream(const WindowsNativeStream &) = delete;
    WindowsNativeStream(WindowsNativeStream &&) = delete;
    auto operator=(const WindowsNativeStream &) -> WindowsNativeStream & = delete;
    auto operator=(WindowsNativeStream &&) -> WindowsNativeStream & = delete;

public: // implement NativeOutputStream
    void writeBytes(std::span<const char> bytes) override;
    void writeText(const text::StringView &text) override;
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
    void writeWideText(std::wstring_view text);

private:
    WindowsNativeHandle _handle{};
    NativeStreamOwnership _ownership{NativeStreamOwnership::Borrowed};
    bool _isConsole{false};
};

}
