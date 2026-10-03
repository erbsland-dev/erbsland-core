// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StopSource_fwd.hpp"

#include <atomic>
#include <memory>

namespace erbsland::util {

/// A shared, read-only observation of a cooperative stop request.
/// A default token never reports stopping. Destroying the source does not request stopping.
/// @tested{StopSourceTest}
class StopToken final {
    friend class StopSource;

public:
    // defaults
    StopToken() = default;

public: // accessors
    /// Test whether the associated source requested stopping.
    [[nodiscard]] auto isStopRequested() const noexcept -> bool;

private:
    /// Retain the source request independently of its signalling handles.
    /// @param state Shared request state, or null for an inert token.
    explicit StopToken(std::shared_ptr<std::atomic<bool>> state);

private:
    std::shared_ptr<std::atomic<bool>> _state; ///< Shared request, or null for a default token.
};

}
