// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CurrentLoop.hpp"

#include "impl/BoundEvent.hpp"
#include "impl/EventMethodHandler.hpp"
#include "impl/WeakEventMethodHandler.hpp"

#include <utility>

namespace erbsland::event {

template <typename tFunction>
    requires impl::EventBindableFunction<tFunction>
[[nodiscard]] auto bindEvent(EventTargetPtr target, tFunction &&callback) {
    return impl::BoundEvent<std::decay_t<tFunction>>{std::move(target), std::forward<tFunction>(callback)};
}

template <typename tFunction>
    requires impl::EventBindableFunction<tFunction>
[[nodiscard]] auto bindEvent(tFunction &&callback) {
    return bindEvent(currentLoop(), std::forward<tFunction>(callback));
}

template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindEvent(EventTargetPtr target, std::shared_ptr<tRecipient> recipient, tMethod method) {
    return bindEvent(std::move(target), impl::EventMethodHandler<tRecipient, tMethod>{std::move(recipient), method});
}

template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindEvent(std::shared_ptr<tRecipient> recipient, tMethod method) {
    return bindEvent(currentLoop(), std::move(recipient), method);
}

template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindWeakEvent(EventTargetPtr target, std::weak_ptr<tRecipient> weakRecipient, tMethod method) {
    return bindEvent(
        std::move(target), impl::WeakEventMethodHandler<tRecipient, tMethod>{std::move(weakRecipient), method});
}

template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindWeakEvent(std::weak_ptr<tRecipient> weakRecipient, tMethod method) {
    return bindWeakEvent(currentLoop(), std::move(weakRecipient), method);
}

}
