// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BindEventTraits.hpp"

#include "../Events_fwd.hpp"

#include <concepts>
#include <memory>
#include <type_traits>

namespace erbsland::event::impl {

/// A reusable callback that transfers owned arguments to one event target.
/// @tparam tFunction The stored handler type.
/// @seeref{event-bound-callbacks}
/// @tested{BindEventTest BindWeakEventTest}
template <typename tFunction>
class BoundEvent final {
public:
    /// Retain a target and construct a shared handler.
    /// @tparam tSource The source handler expression type.
    /// @param events The non-null target.
    /// @param function The handler to copy or move into shared storage. Nullable handler types must not be empty.
    /// @throws Any exception from allocating or constructing the handler.
    template <typename tSource>
        requires std::constructible_from<tFunction, tSource>
    explicit BoundEvent(EventsPtr events, tSource &&function);

    // defaults
    BoundEvent(const BoundEvent &) = default;
    BoundEvent(BoundEvent &&) noexcept = default;
    auto operator=(const BoundEvent &) -> BoundEvent & = default;
    auto operator=(BoundEvent &&) noexcept -> BoundEvent & = default;

public:
    /// Queue an independent invocation using copies of lvalues and moved rvalues.
    /// This binding must not have been moved from.
    /// @tparam tArguments The supplied argument expression types.
    /// @param arguments Arguments to store and deliver as rvalues on the target loop.
    /// @throws Any exception from argument storage, allocation, or target submission.
    template <typename... tArguments>
        requires(std::constructible_from<std::decay_t<tArguments>, tArguments> && ...) &&
        EventVoidInvocable<tFunction &, std::decay_t<tArguments> &&...>
    void operator()(tArguments &&...arguments) const;

private:
    EventsPtr _events;                    ///< Target retained by bindings, not by queued transport closures.
    std::shared_ptr<tFunction> _function; ///< Handler shared by binding copies and pending deliveries.
};

}

#include "BoundEvent.tpp"
