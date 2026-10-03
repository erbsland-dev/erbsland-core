// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../err/ParameterFailure.hpp"
#include "../../text/Literals.hpp"

#include <functional>
#include <utility>

namespace erbsland::event::impl {

template <typename tRecipient, typename tMethod>
WeakEventMethodHandler<tRecipient, tMethod>::WeakEventMethodHandler(
    std::weak_ptr<tRecipient> recipient, tMethod method) :
    _recipient{std::move(recipient)}, _method{method} {
    using namespace text::literals;
    if (_method == nullptr) {
        throw err::ParameterFailure{"The event method must not be null."_el, "method"_el};
    }
}

template <typename tRecipient, typename tMethod>
template <typename... tArguments>
    requires EventVoidInvocable<tMethod &, tRecipient &, tArguments...>
void WeakEventMethodHandler<tRecipient, tMethod>::operator()(tArguments &&...arguments) {
    // This handler runs on the receiving loop, so submission never prolongs recipient lifetime.
    if (const auto recipient = _recipient.lock()) {
        std::invoke(_method, *recipient, std::forward<tArguments>(arguments)...);
    }
}

}
