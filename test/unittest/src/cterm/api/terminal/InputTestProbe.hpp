// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

#include <chrono>

class InputProbe final : public Input {
public:
    [[nodiscard]] auto mode() const noexcept -> Mode override { return _mode; }
    void setMode(const Mode mode) override { _mode = mode; }
    [[nodiscard]] auto readLine() -> erbsland::text::String override { return erbsland::text::String{_line}; }

protected:
    [[nodiscard]] auto readKeyImpl(const std::chrono::milliseconds timeout) const -> Key override {
        _lastTimeout = timeout;
        _readKeyCallCount += 1;
        return _readResult;
    }

    [[nodiscard]] auto waitForKeyImpl() const -> Key override {
        _waitForKeyCallCount += 1;
        return _waitResult;
    }

public:
    Mode _mode{Mode::ReadLine};
    std::string _line{"typed line"};
    mutable std::chrono::milliseconds _lastTimeout{-1};
    mutable int _readKeyCallCount{0};
    mutable int _waitForKeyCallCount{0};
    Key _readResult{Key::Escape};
    Key _waitResult{Key::Enter};
};
