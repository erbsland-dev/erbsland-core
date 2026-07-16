// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Literals.hpp"
#include "../../Input.hpp"
#include "../../RegExError.hpp"

#include <chrono>
#include <cstddef>

namespace erbsland::re::impl {

using namespace text::literals;

/// A watchdog that detects stuck engines and enforces optional timeouts.
class Watchdog final {
public:
    /// Reset the watchdog state for a new engine run.
    void reset() noexcept { _positionAtLastTimeoutCheck = 0; }

    /// Set a timeout.
    void setTimeout(const std::chrono::milliseconds timeout) {
        _hasTimeoutTime = true;
        _startTime = std::chrono::steady_clock::now();
        _timeoutTime = _startTime + timeout;
    }

    /// Increase the tick count and check if we run into a timeout.
    void tick(const InputPosition currentPosition) {
        ++_tick;
        if ((_tick & 0xFFFFU) == 0) { // do infrequent checks for a timeout
            if (_positionAtLastTimeoutCheck == currentPosition) {
                // It seems the engine got stuck. It's unlikely the input stalls for >60'000 commands.
                throw RegExError{
                    ErrorCategory::Timeout,
                    "Failed to match regular expression"_el,
                    "The engine stopped making progress."_el};
            }
            _positionAtLastTimeoutCheck = currentPosition;
            if (_hasTimeoutTime) {
                const auto now = std::chrono::steady_clock::now();
                if (now >= _timeoutTime) {
                    throw RegExError{
                        ErrorCategory::Timeout,
                        "Failed to match regular expression"_el,
                        "The configured matching timeout was reached."_el};
                }
            }
        }
    }

public:
    /// Access the current tick count.
    [[nodiscard]] auto tickCount() const noexcept -> std::size_t { return _tick; }
    /// Access the input position when we did the last timeout check.
    [[nodiscard]] auto positionAtLastTimeoutCheck() const noexcept -> InputPosition {
        return _positionAtLastTimeoutCheck;
    }

private:
    bool _hasTimeoutTime{false};                        ///< If a timeout time is set.
    std::size_t _tick{0};                               ///< Counting ticks
    std::chrono::steady_clock::time_point _startTime;   ///< The start time of the engine (if a timeout is set).
    std::chrono::steady_clock::time_point _timeoutTime; ///< The timeout time of the engine (if a timeout is set).
    InputPosition _positionAtLastTimeoutCheck{};        ///< The input position when we did the last timeout check.
};

}
