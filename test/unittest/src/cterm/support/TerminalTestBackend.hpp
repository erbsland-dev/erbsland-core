// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TerminalTestCursorMove.hpp"

#include <erbsland/text/StringConverter.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

/// Recording terminal backend used by terminal unit tests.
/// @notest{Used only by terminal unit tests.}
class TerminalTestBackend final : public Backend {
public:
    using CursorMove = TerminalTestCursorMove;

public: // implement Backend
    /// Record platform initialization.
    void initializePlatform() override { _initializePlatformCallCount += 1; }
    /// Record platform restoration.
    void restorePlatform() override { _restorePlatformCallCount += 1; }
    /// Report whether color-code support is enabled for the test.
    [[nodiscard]] auto supportsColorCodes() const noexcept -> bool override { return _supportsColorCodes; }
    /// Report whether cursor-code support is enabled for the test.
    [[nodiscard]] auto supportsCursorCodes() const noexcept -> bool override { return _supportsCursorCodes; }
    [[nodiscard]] auto supportsCursorVisibilityCodes() const noexcept -> bool override {
        return _supportsCursorVisibilityCodes;
    }
    [[nodiscard]] auto supportsAlternateScreenBufferCodes() const noexcept -> bool override {
        return _supportsAlternateScreenBufferCodes;
    }
    [[nodiscard]] auto supportedBlockAttributes() const noexcept -> BlockAttributes override {
        return _supportedBlockAttributes;
    }
    [[nodiscard]] auto supportedBlockAttributeCodes() const noexcept -> BlockAttributes override {
        return _supportedBlockAttributeCodes;
    }
    [[nodiscard]] auto isInteractive() const noexcept -> bool override { return _isInteractive; }
    [[nodiscard]] auto detectScreenSize() -> std::optional<block::Size> override {
        _detectScreenSizeCallCount += 1;
        return _detectedScreenSize;
    }
    void emitColor(const Color color) override { _emittedColors.push_back(color); }
    void emitBlockAttributes(const BlockAttributes attributes) override {
        _emittedBlockAttributes.push_back(attributes);
    }
    void moveCursor(const block::Position pos, const MoveMode mode) override {
        _cursorMoves.push_back(CursorMove{pos, mode});
    }
    void clearScreen() override { _clearScreenCallCount += 1; }
    void setCursorVisible(const bool visible) override { _cursorVisibilityChanges.push_back(visible); }
    void setAlternateScreenBuffer(const bool enabled) override {
        _alternateScreenBufferChanges.push_back(enabled);
        _isAlternateScreenActive = enabled;
    }
    void emitText(const erbsland::text::String &text) override {
        _emittedText.emplace_back(erbsland::text::StringConverter{text}.toStdString());
    }
    void emitFlush() override { _emitFlushCallCount += 1; }
    [[nodiscard]] auto inputMode() const noexcept -> Input::Mode override { return _inputMode; }
    void setInputMode(const Input::Mode mode) override {
        _setInputModeCallCount += 1;
        _inputMode = mode;
    }
    /// Record a key-read request and return its configured result.
    [[nodiscard]] auto readKey(const std::chrono::milliseconds timeout = {}) -> Key override {
        _readKeyCallCount += 1;
        _readKeyTimeouts.push_back(timeout);
        if (_readKeyResults.empty()) {
            return {};
        }
        const auto result = _readKeyResults.front();
        _readKeyResults.pop();
        return result;
    }
    [[nodiscard]] auto waitForKey() -> Key override {
        _waitForKeyCallCount += 1;
        if (_waitForKeyResults.empty()) {
            return {};
        }
        const auto result = _waitForKeyResults.front();
        _waitForKeyResults.pop();
        return result;
    }
    [[nodiscard]] auto readLine() -> erbsland::text::String override {
        _readLineCallCount += 1;
        if (_readLineResults.empty()) {
            return {};
        }
        auto result = _readLineResults.front();
        _readLineResults.pop();
        return result;
    }

public:
    /// Return the text emitted by the backend.
    [[nodiscard]] auto output() const -> std::string {
        auto result = std::string{};
        for (const auto &segment : _emittedText) {
            result += segment;
        }
        return result;
    }

    /// Clear the recorded emitted text.
    void clearOutput() { _emittedText.clear(); }

    /// Clear all recorded backend operations and results.
    void clearRecordedOperations() {
        _emittedText.clear();
        _emittedColors.clear();
        _emittedBlockAttributes.clear();
        _cursorMoves.clear();
        _clearScreenCallCount = 0;
        _cursorVisibilityChanges.clear();
        _alternateScreenBufferChanges.clear();
        _emitFlushCallCount = 0;
        _setInputModeCallCount = 0;
        _readKeyCallCount = 0;
        _readKeyTimeouts.clear();
        _waitForKeyCallCount = 0;
        _readLineCallCount = 0;
    }

public:
    bool _supportsColorCodes = true;
    bool _supportsCursorCodes = true;
    bool _supportsCursorVisibilityCodes = true;
    bool _supportsAlternateScreenBufferCodes = true;
    BlockAttributes _supportedBlockAttributes = BlockAttributes::all();
    BlockAttributes _supportedBlockAttributeCodes = BlockAttributes::all();
    bool _isInteractive = true;
    bool _isAlternateScreenActive = false;
    std::optional<block::Size> _detectedScreenSize{};
    Input::Mode _inputMode = Input::Mode::ReadLine;
    int _initializePlatformCallCount = 0;
    int _restorePlatformCallCount = 0;
    int _detectScreenSizeCallCount = 0;
    int _emitFlushCallCount = 0;
    int _clearScreenCallCount = 0;
    int _setInputModeCallCount = 0;
    int _readKeyCallCount = 0;
    int _waitForKeyCallCount = 0;
    int _readLineCallCount = 0;
    std::vector<Color> _emittedColors;
    std::vector<BlockAttributes> _emittedBlockAttributes;
    std::vector<CursorMove> _cursorMoves;
    std::vector<bool> _cursorVisibilityChanges;
    std::vector<bool> _alternateScreenBufferChanges;
    std::vector<std::chrono::milliseconds> _readKeyTimeouts;
    std::queue<Key> _readKeyResults;
    std::queue<Key> _waitForKeyResults;
    std::queue<erbsland::text::String> _readLineResults;
    std::vector<std::string> _emittedText;
};
