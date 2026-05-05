// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Backend.hpp"

#include "../../text/StringConverter.hpp"

#include <atomic>
#include <cassert>
#include <string>

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
    void setCachingEnabled(bool enabled) noexcept {
        if (_cachingEnabled != enabled) {
            _cachingEnabled = enabled;
            if (enabled) {
                _buffer.reserve(cEnabledSize);
            } else {
                handleEmit(true);
            }
        }
    }

    /// Test if caching is enabled.
    [[nodiscard]] auto cachingEnabled() const noexcept -> bool { return _cachingEnabled; }

    /// Write the given text to the line buffer.
    /// This function scans each added text segment for a NL, and sets the newline flag.
    void write(const std::string_view text) noexcept {
        _buffer += text;
        if (text.rfind('\n') != std::string::npos) {
            _hasNewLine = true;
        }
    }

    /// Append the given character to the line buffer (not the colors).
    void write(const Block character) noexcept {
        _buffer += text::StringConverter{character.charStr()}.toStdString();
        if (character == U'\n') {
            _hasNewLine = true;
        }
    }

    /// Test if we have line buffering enabled and emit its contents.
    /// If the line buffer is disabled, this method will emit its contents at every call.
    /// If the line buffer is enabled, this method will emit its contents when there is a NL in the buffer.
    /// @param forceEmit If true, the line buffer will be emitted regardless of the buffer content.
    void handleEmit(const bool forceEmit = false) noexcept {
        auto fullLineBufferEmit = [&]() -> void {
            _backend->emitText(text::String{_buffer});
            _buffer.clear();
            _hasNewLine = false;
        };
        if (!_cachingEnabled || forceEmit) {
            fullLineBufferEmit();
        }
        if (_buffer.size() >= cMaxSize) { // force emit if the line buffer gets too large.
            fullLineBufferEmit();
            return;
        }
        if (_emitLock.isLocked() || !_hasNewLine) {
            return;
        }
        const auto nlPos = _buffer.rfind('\n');
        if (nlPos == std::string::npos) { // coverage: should never happen.
            fullLineBufferEmit();
            return;
        }
        // partial emit
        _backend->emitText(text::String{std::string_view{_buffer}.substr(0, nlPos + 1)});
        _buffer.erase(0, nlPos + 1);
    }

    /// Free allocated memory and emit remaining contents.
    void shutdown() noexcept {
        handleEmit(true);
        _buffer.clear();
        _buffer.shrink_to_fit();
    }

private:
    struct EmitLock {
        auto isLocked() const noexcept -> bool { return _emitLockCount > 0; }
        void lock() noexcept {
            assert(_emitLockCount < 100'000); // detect misuse
            _emitLockCount += 1;
        }
        void unlock() noexcept {
            assert(_emitLockCount > 0); // detect misuse
            _emitLockCount -= 1;
        }
        std::atomic<std::size_t> _emitLockCount{0}; ///< >0 locks the emitting
    };

private:
    BackendPtr _backend{nullptr}; ///< The backend to emit to.
    bool _cachingEnabled{true};   ///< If line buffering is enabled.
    std::string _buffer;          ///< The text buffer.
    bool _hasNewLine{false};      ///< A flag if there is a NL in the line buffer.
    EmitLock _emitLock;           ///< The emit lock.
};

class LineBuffer::EmitLockGuard {
public:
    explicit EmitLockGuard(LineBuffer &lineBuffer) : _lineBuffer{lineBuffer} { _lineBuffer._emitLock.lock(); }
    ~EmitLockGuard() { _lineBuffer._emitLock.unlock(); }

    // delete copy/move/assign
    EmitLockGuard(const EmitLockGuard &) = delete;
    EmitLockGuard(EmitLockGuard &&) = delete;
    auto operator=(const EmitLockGuard &) -> EmitLockGuard & = delete;
    auto operator=(EmitLockGuard &&) -> EmitLockGuard & = delete;

private:
    LineBuffer &_lineBuffer;
};

}
