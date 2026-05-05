// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace erbsland::event::impl {

/// Shared cooperative stop state for tokens and sources.
class StopState final {
public:
    std::atomic<bool> stopRequested{false}; ///< Set once a cooperative stop was requested.
};

}

namespace erbsland::event {

class StopSource;

/// Cooperative stop token passed to stoppable UI callbacks.
class StopToken final {
    friend class StopSource;

public:
    /// Create an empty stop token that can never observe stop requests.
    StopToken() noexcept = default;

    // defaults
    ~StopToken() = default;
    StopToken(const StopToken &) noexcept = default;
    StopToken(StopToken &&) noexcept = default;
    auto operator=(const StopToken &) noexcept -> StopToken & = default;
    auto operator=(StopToken &&) noexcept -> StopToken & = default;

public:
    /// Test if this token is connected to a stop source.
    /// @return `true` if stop requests can be observed.
    [[nodiscard]] auto stopPossible() const noexcept -> bool { return _state != nullptr; }
    /// Test if a cooperative stop was requested.
    /// @return `true` if the associated stop source requested a stop.
    [[nodiscard]] auto stopRequested() const noexcept {
        if (_state == nullptr) {
            return false;
        }
        return _state->stopRequested.load(std::memory_order_acquire);
    }

private:
    /// Create a stop token for the given shared state.
    /// @param state The shared stop state.
    explicit StopToken(std::shared_ptr<impl::StopState> state) noexcept : _state{std::move(state)} {}

private:
    std::shared_ptr<impl::StopState> _state; ///< The shared stop state.
};

/// Stop source used by the event system to request a cooperative shutdown.
class StopSource final {
public:
    /// Create a new stop source with its own shared stop state.
    StopSource() : _state{std::make_shared<impl::StopState>()} {}

    // defaults
    ~StopSource() = default;
    StopSource(const StopSource &) noexcept = default;
    StopSource(StopSource &&) noexcept = default;
    auto operator=(const StopSource &) noexcept -> StopSource & = default;
    auto operator=(StopSource &&) noexcept -> StopSource & = default;

public:
    /// Create a token that observes this source.
    /// @return A stop token connected to this source.
    [[nodiscard]] auto getToken() const noexcept -> StopToken { return StopToken{_state}; }
    /// Test if this source is connected to a shared stop state.
    /// @return `true` if stop requests are supported.
    [[nodiscard]] auto stopPossible() const noexcept -> bool { return _state != nullptr; }
    /// Test if a cooperative stop was already requested.
    /// @return `true` if `requestStop()` was called before.
    [[nodiscard]] auto stopRequested() const noexcept {
        if (_state == nullptr) {
            return false;
        }
        return _state->stopRequested.load(std::memory_order_acquire);
    }
    /// Request a cooperative stop.
    /// @return `true` if this call changed the stop state.
    auto requestStop() noexcept -> bool {
        if (_state == nullptr) {
            return false;
        }
        return !_state->stopRequested.exchange(true, std::memory_order_acq_rel);
    }

private:
    std::shared_ptr<impl::StopState> _state; ///< The shared stop state.
};

}
