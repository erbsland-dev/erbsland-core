// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPart_fwd.hpp"
#include "ApplicationPartIdentifier_fwd.hpp"
#include "ApplicationPartManager_fwd.hpp"
#include "ApplicationPartManagerState.hpp"
#include "ApplicationPartState.hpp"
#include "ApplicationPartTraits.hpp"

#include "../err/LogicError.hpp"
#include "../text/Literals.hpp"

#include <memory>

namespace erbsland::core {

/// Thread-safe read and wait access to an application-part manager.
/// @seedoc{/reference/core/application_parts}
/// @tested{ApplicationPartManagerTest ApplicationPartApplicationTest}
class ApplicationPartManagerAccess {
public:
    // defaults
    virtual ~ApplicationPartManagerAccess() = default;

public: // state
    /// Get the manager lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> ApplicationPartManagerState = 0;
    /// Get one part's lifecycle state.
    /// @param identifier The part identifier to resolve by name.
    /// @return The current part state.
    /// @throws err::LogicError If the manager is not prepared or the identifier is unknown.
    [[nodiscard]] virtual auto partState(const ApplicationPartIdentifierPtr &identifier) const
        -> ApplicationPartState = 0;
    /// Wait until the manager runs or reaches a terminal state.
    /// @return `true` if `Running` was reached; `false` for terminal failure or shutdown.
    [[nodiscard]] virtual auto waitForRunning() -> bool = 0;
    /// Wait until the manager stops or fails.
    /// @return `true` for `Stopped`; `false` for `Failed`.
    [[nodiscard]] virtual auto waitForStopped() -> bool = 0;
    /// Wait until one part runs or reaches a terminal state.
    /// @param identifier The part identifier to resolve by name.
    /// @return `true` if `Running` was reached; `false` for terminal failure or shutdown.
    [[nodiscard]] virtual auto waitForRunning(const ApplicationPartIdentifierPtr &identifier) -> bool = 0;
    /// Wait until one part stops or fails.
    /// @param identifier The part identifier to resolve by name.
    /// @return `true` for `Stopped`; `false` for `Failed`.
    [[nodiscard]] virtual auto waitForStopped(const ApplicationPartIdentifierPtr &identifier) -> bool = 0;

public: // lookup
    /// Access one prepared part by identifier.
    /// @param identifier The part identifier to resolve by name.
    /// @return The prepared application part.
    /// @throws err::LogicError If the manager is not prepared or the identifier is unknown.
    [[nodiscard]] virtual auto part(const ApplicationPartIdentifierPtr &identifier) const -> ApplicationPartPtr = 0;
    /// Access one prepared part through its public interface.
    /// @tparam T The abstract part interface.
    /// @return The prepared part implementing `T`.
    /// @throws err::LogicError If the manager is not prepared, the identifier is unknown, or the registered part does
    /// not implement `T`.
    template <ApplicationPartInterface T>
    [[nodiscard]] auto part() const -> std::shared_ptr<T> {
        using namespace text::literals;
        const auto result = std::dynamic_pointer_cast<T>(part(T::partIdentifier()));
        if (result == nullptr) {
            throw err::LogicError{"The registered application part does not implement the requested interface."_el};
        }
        return result;
    }
};

}
