// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/block/Size.hpp>
#include <erbsland/cterm/Backend.hpp>
#include <erbsland/cterm/Input.hpp>
#include <erbsland/cterm/Key.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/TextOutputStream.hpp>
#include <erbsland/TimeAmounts.hpp>

#include <optional>
#include <utility>

/// Route terminal output into a captured stream, regardless of the native terminal.
/// @notest{Unit-test fixture; exercised by ToolApplicationTest.}
class CapturedTerminalBackend final : public el::cterm::Backend {
public:
    /// Create a backend writing to the captured output stream.
    /// @param interactive Enable ANSI output for renderer tests.
    /// @param output The stream that receives terminal output.
    explicit CapturedTerminalBackend(el::stream::TextOutputStreamPtr output, bool interactive = false) :
        _interactive{interactive}, _output{std::move(output)} {}

public: // implement Backend
    void initializePlatform() override {}
    void restorePlatform() override {}
    auto supportsColorCodes() const noexcept -> bool override { return _interactive; }
    auto supportsCursorCodes() const noexcept -> bool override { return _interactive; }
    auto isInteractive() const noexcept -> bool override { return _interactive; }
    auto detectScreenSize() -> std::optional<el::block::Size> override { return {}; }
    void emitText(const el::String &text) override { _output->write(text); }
    void emitFlush() override { _output->flush(); }
    auto inputMode() const noexcept -> el::cterm::Input::Mode override { return el::cterm::Input::Mode::ReadLine; }
    void setInputMode(el::cterm::Input::Mode) override {}
    auto readKey(el::time::Milliseconds) -> el::cterm::Key override {
        throw el::LogicFailure{"Unexpected terminal input in a tool test."};
    }
    auto waitForKey() -> el::cterm::Key override {
        throw el::LogicFailure{"Unexpected terminal input in a tool test."};
    }
    auto readLine() -> el::String override { throw el::LogicFailure{"Unexpected terminal input in a tool test."}; }

private:
    bool _interactive{};                     ///< Enables native-like ANSI rendering in tests.
    el::stream::TextOutputStreamPtr _output; ///< Captured terminal destination.
};
