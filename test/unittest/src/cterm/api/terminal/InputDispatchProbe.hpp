// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

#include <chrono>
#include <string>

class InputDispatchProbe final : public Input {
public:
    [[nodiscard]] auto mode() const noexcept -> Mode override { return _mode; }

    void setMode(const Mode mode) override { _mode = mode; }

    [[nodiscard]] auto readLine() -> erbsland::text::String override { return erbsland::text::String{_readLineResult}; }

public:
    Mode _mode = Mode::ReadLine;
    mutable std::chrono::milliseconds _lastTimeout{};
    mutable bool _waitForKeyWasCalled = false;
    Key _readResult{Key::Escape};
    Key _waitForKeyResult{Key::Enter};
    std::string _readLineResult = "line";

protected:
    [[nodiscard]] auto readKeyImpl(const std::chrono::milliseconds timeout) const -> Key override {
        _lastTimeout = timeout;
        return _readResult;
    }

    [[nodiscard]] auto waitForKeyImpl() const -> Key override {
        _waitForKeyWasCalled = true;
        return _waitForKeyResult;
    }
};
