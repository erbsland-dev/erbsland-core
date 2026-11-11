// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EventTarget.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../text/Literals.hpp"

#include <tuple>
#include <utility>

namespace erbsland::event::impl {

template <typename tFunction>
template <typename tSource>
    requires std::constructible_from<tFunction, tSource>
BoundEvent<tFunction>::BoundEvent(EventTargetPtr events, tSource &&function) : _events{std::move(events)} {
    using namespace text::literals;
    if (_events == nullptr) {
        throw err::ParameterFailure{"The event target must not be null."_el, "events"_el};
    }
    if constexpr (std::is_pointer_v<tFunction> || IsEventStdFunction<tFunction>::value) {
        if (!function) {
            throw err::ParameterFailure{"The event callback must not be empty."_el, "callback"_el};
        }
    }
    // Copies share handler identity; even move-only captures remain reusable on the target loop.
    _function = std::make_shared<tFunction>(std::forward<tSource>(function));
}

template <typename tFunction>
template <typename... tArguments>
    requires(std::constructible_from<std::decay_t<tArguments>, tArguments> && ...) &&
    EventVoidInvocable<tFunction &, std::decay_t<tArguments> &&...>
void BoundEvent<tFunction>::operator()(tArguments &&...arguments) const {
    using namespace text::literals;
    if (_events == nullptr || _function == nullptr) {
        throw err::LogicFailure{"Cannot invoke a moved-from event binding."_el};
    }
    // Shared packet storage makes the queued closure copyable even for move-only arguments.
    auto packet = std::make_shared<std::tuple<std::decay_t<tArguments>...>>(std::forward<tArguments>(arguments)...);
    // Do not capture this binding or its target: the target already owns the queued closure.
    _events->invoke(
        [function = _function, packet = std::move(packet)]() -> void { std::apply(*function, std::move(*packet)); });
}

}
