// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeByteStream.hpp"
#include "NativeOutputStream.hpp"

#include "../StreamErrorContext.hpp"

#include "../../system/WindowsErrorContext.hpp"

#include <atomic>
#include <mutex>
#include <vector>

namespace erbsland::stream::impl {

/// A native Windows stream handle.
using WindowsNativeHandle = void *;

/// Native byte stream wrapper for Windows handles.
/// @tested{WindowsNativeStreamTest}
class WindowsNativeStream final : public NativeOutputStream, public NativeByteStream {
    class Operation final {
    public:
        explicit Operation(const WindowsNativeStream &stream);
        ~Operation();

        // deletions
        Operation(const Operation &) = delete;
        Operation(Operation &&) = delete;
        auto operator=(const Operation &) -> Operation & = delete;
        auto operator=(Operation &&) -> Operation & = delete;

    public: // accessors
        [[nodiscard]] auto handle() const noexcept -> WindowsNativeHandle { return _handle; }

    private:
        const WindowsNativeStream &_stream;
        WindowsNativeHandle _handle{};
        WindowsNativeHandle _threadHandle{};
    };

public:
    /// Create a Windows native stream wrapper.
    /// @param handle The handle to wrap.
    /// @param ownership If the wrapper owns the handle.
    /// @param path The path represented by the handle, if available.
    explicit WindowsNativeStream(
        WindowsNativeHandle handle,
        NativeStreamOwnership ownership,
        text::String path = {},
        bool positioningAllowed = true);

    // defaults
    ~WindowsNativeStream() override;
    WindowsNativeStream(const WindowsNativeStream &) = delete;
    WindowsNativeStream(WindowsNativeStream &&) = delete;
    auto operator=(const WindowsNativeStream &) -> WindowsNativeStream & = delete;
    auto operator=(WindowsNativeStream &&) -> WindowsNativeStream & = delete;

public: // implement NativeOutputStream
    using StreamErrorSource::throwError;

    void writeBytes(std::span<const char> bytes) override;
    void writeText(const text::String &text) override;
    void flush() override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;

public: // implement NativeByteStream
    [[nodiscard]] auto supportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto position() const -> unit::ByteIndex override;
    auto setPosition(unit::ByteIndex position) -> unit::ByteIndex override;
    auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> unit::ByteIndex override;
    void close() override;
    void abort() noexcept override;
    [[nodiscard]] auto read(std::span<mem::Byte> destination) -> unit::ByteLength override;
    void write(std::span<const mem::Byte> bytes) override;

public:
    [[nodiscard]] auto isOpen() const noexcept -> bool;

public:
    /// Get the current file size if this stream references a file.
    /// @return The file size in bytes, or zero for zero-length streams.
    /// @throws stream::StreamError If the stream is closed or size lookup fails.
    [[nodiscard]] auto fileSize() const -> unit::ByteLength;

private:
    [[noreturn]] void throwError(
        text::String title, text::String description, system::WindowsErrorContext::ErrorCode errorCode) const;
    [[noreturn]] void throwErrorFromLastError(text::String title, text::String description) const;
    void writeWideText(std::wstring_view text);
    void finishOperation(WindowsNativeHandle threadHandle) const noexcept;

private:
    std::atomic<WindowsNativeHandle> _handle{};                        ///< Wrapped native handle.
    NativeStreamOwnership _ownership{NativeStreamOwnership::Borrowed}; ///< Handle ownership mode.
    text::String _path;                                                ///< Stream path, when available.
    bool _isConsole{false};                                            ///< Whether the handle is a Windows console.
    bool _supportsPositioning{false};                                  ///< Whether this handle can be positioned.
    mutable std::mutex _operationMutex;                                ///< Protects operation and deferred close state.
    mutable std::vector<WindowsNativeHandle> _operationThreads;        ///< Active operation threads.
    mutable WindowsNativeHandle _deferredCloseHandle{};                ///< Deferred owned handle.
};

}
