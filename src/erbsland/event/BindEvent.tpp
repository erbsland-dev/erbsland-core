// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CurrentEvents.hpp"

#include "impl/BoundEvent.hpp"
#include "impl/EventMethodHandler.hpp"
#include "impl/WeakEventMethodHandler.hpp"

#include <utility>

namespace erbsland::event {

template <typename tFunction>
    requires impl::EventBindableFunction<tFunction>
[[nodiscard]] auto bindEvent(EventsPtr events, tFunction &&callback) {
    return impl::BoundEvent<std::decay_t<tFunction>>{std::move(events), std::forward<tFunction>(callback)};
}

template <typename tFunction>
    requires impl::EventBindableFunction<tFunction>
[[nodiscard]] auto bindEvent(tFunction &&callback) {
    return bindEvent(currentEvents(), std::forward<tFunction>(callback));
}

template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindEvent(EventsPtr events, std::shared_ptr<tRecipient> recipient, tMethod method) {
    return bindEvent(std::move(events), impl::EventMethodHandler<tRecipient, tMethod>{std::move(recipient), method});
}

template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindEvent(std::shared_ptr<tRecipient> recipient, tMethod method) {
    return bindEvent(currentEvents(), std::move(recipient), method);
}

template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindWeakEvent(EventsPtr events, std::weak_ptr<tRecipient> weakRecipient, tMethod method) {
    return bindEvent(
        std::move(events), impl::WeakEventMethodHandler<tRecipient, tMethod>{std::move(weakRecipient), method});
}

template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindWeakEvent(std::weak_ptr<tRecipient> weakRecipient, tMethod method) {
    return bindWeakEvent(currentEvents(), std::move(weakRecipient), method);
}

}
