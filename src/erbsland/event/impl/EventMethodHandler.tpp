// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../err/ParameterFailure.hpp"
#include "../../text/Literals.hpp"

#include <functional>
#include <utility>

namespace erbsland::event::impl {

template <typename tRecipient, typename tMethod>
EventMethodHandler<tRecipient, tMethod>::EventMethodHandler(std::shared_ptr<tRecipient> recipient, tMethod method) :
    _recipient{std::move(recipient)}, _method{method} {
    using namespace text::literals;
    if (_recipient == nullptr) {
        throw err::ParameterFailure{"The event recipient must not be null."_el, "recipient"_el};
    }
    if (_method == nullptr) {
        throw err::ParameterFailure{"The event method must not be null."_el, "method"_el};
    }
}

template <typename tRecipient, typename tMethod>
template <typename... tArguments>
    requires EventVoidInvocable<tMethod &, tRecipient &, tArguments...>
void EventMethodHandler<tRecipient, tMethod>::operator()(tArguments &&...arguments) {
    std::invoke(_method, *_recipient, std::forward<tArguments>(arguments)...);
}

}
