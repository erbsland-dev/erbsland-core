// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Backend.hpp"
#include "../Block.hpp"

#include "../../text/CharSet.hpp"
#include "../../text/String.hpp"
#include "../../text/StringView.hpp"

#include <atomic>
#include <cstddef>

namespace erbsland::cterm::impl {

/// A smart line buffer.
class LineBuffer {
public:
    class EmitLockGuard;
    friend class EmitLockGuard;

public:
    /// A minimum buffer size reserved for low-level operations.
    constexpr static std::size_t cDisabledSize = 1024;
    /// A minimum buffer size reserver if the line buffer is enabled.
    constexpr static std::size_t cEnabledSize = 4096;
    /// The maximum size of the line buffer before it is force-emitted.
    constexpr static std::size_t cMaxSize = 1023 * 1024; // 1k smaller than 1MB

public:
    LineBuffer() = default;

public:
    /// Set the backend.
    void setBackend(const BackendPtr &backend) noexcept { _backend = backend; }

    /// Enable or disable line caching.
    /// @param enabled true to enable caching, false to disable
    void setCachingEnabled(bool enabled) noexcept;

    /// Test if caching is enabled.
    [[nodiscard]] auto cachingEnabled() const noexcept -> bool { return _cachingEnabled; }

    /// Write the given text to the line buffer.
    /// This function scans each added text segment for a NL, and sets the newline flag.
    void write(const text::StringView &text) noexcept;

    /// Append the given character to the line buffer (not the colors).
    void write(Block character) noexcept;

    /// Test if we have line buffering enabled and emit its contents.
    /// If the line buffer is disabled, this method will emit its contents at every call.
    /// If the line buffer is enabled, this method will emit its contents when there is a NL in the buffer.
    /// @param forceEmit If true, the line buffer will be emitted regardless of the buffer content.
    void handleEmit(bool forceEmit = false) noexcept;

    /// Free allocated memory and emit remaining contents.
    void shutdown() noexcept;

private:
    [[nodiscard]] static auto newLineCharacters() -> const text::CharSet &;
    void emitFullLineBuffer() noexcept;

    struct EmitLock {
        [[nodiscard]] auto isLocked() const noexcept -> bool;
        void lock() noexcept;
        void unlock() noexcept;
        std::atomic<std::size_t> _emitLockCount{0}; ///< >0 locks the emitting
    };

private:
    BackendPtr _backend{nullptr}; ///< The backend to emit to.
    bool _cachingEnabled{true};   ///< If line buffering is enabled.
    text::String _buffer;         ///< The text buffer.
    bool _hasNewLine{false};      ///< A flag if there is a NL in the line buffer.
    EmitLock _emitLock;           ///< The emit lock.
};

class LineBuffer::EmitLockGuard {
public:
    explicit EmitLockGuard(LineBuffer &lineBuffer);
    ~EmitLockGuard();

    // delete copy/move/assign
    EmitLockGuard(const EmitLockGuard &) = delete;
    EmitLockGuard(EmitLockGuard &&) = delete;
    auto operator=(const EmitLockGuard &) -> EmitLockGuard & = delete;
    auto operator=(EmitLockGuard &&) -> EmitLockGuard & = delete;

private:
    LineBuffer &_lineBuffer;
};

}
