// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StopSource_fwd.hpp"
#include "StopToken.hpp"

#include <atomic>
#include <memory>

namespace erbsland::util {

/// A copyable shared source of irreversible, cooperative stop requests.
/// All copies signal the same state. Destruction does not request stopping.
/// @tested{StopSourceTest}
class StopSource final {
public:
    /// Create a source with no stop request.
    StopSource();

public: // accessors
    /// Get an observation token that can outlive this source.
    [[nodiscard]] auto stopToken() const noexcept -> StopToken;
    /// Test whether any copy requested stopping.
    [[nodiscard]] auto isStopRequested() const noexcept -> bool;

public: // signalling
    /// Request stopping without waiting for observers.
    /// @return True only for the call that first requests stopping; false for a moved-from source.
    auto requestStop() noexcept -> bool;

private:
    std::shared_ptr<std::atomic<bool>> _state; ///< Shared request state.
};

}
