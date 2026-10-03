// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Backend.hpp"

#include "../../stream/impl/NativeOutputStream.hpp"

namespace erbsland::cterm::impl {

/// An ANSI stdout backend with explicit screen dimensions and no input.
/// @tested{VirtualBackendTest DocumentationCaptureFullTest}
class VirtualBackend final : public Backend {
public:
    /// Create an independent backend writing raw bytes to native stdout.
    VirtualBackend();

public: // implement Backend
    void initializePlatform() override;
    void restorePlatform() override;
    [[nodiscard]] auto supportsColorCodes() const noexcept -> bool override { return true; }
    [[nodiscard]] auto supportsCursorCodes() const noexcept -> bool override { return true; }
    [[nodiscard]] auto isInteractive() const noexcept -> bool override { return _initialized; }
    [[nodiscard]] auto detectScreenSize() -> std::optional<block::Size> override { return std::nullopt; }
    void emitText(const text::String &text) override;
    void emitFlush() override;
    [[nodiscard]] auto inputMode() const noexcept -> Input::Mode override { return _inputMode; }
    void setInputMode(Input::Mode mode) override { _inputMode = mode; }
    [[nodiscard]] auto readKey(time::Milliseconds timeout) -> Key override;
    [[nodiscard]] auto waitForKey() -> Key override { return {}; }
    [[nodiscard]] auto readLine() -> text::String override { return {}; }

private:
    stream::impl::NativeOutputStreamPtr _output;   ///< Borrowed native stdout, bypassing standard-stream routing.
    Input::Mode _inputMode{Input::Mode::ReadLine}; ///< Requested input mode; input is always empty.
    bool _initialized{false};                      ///< Whether virtual terminal control is active.
};

}
