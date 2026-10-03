// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Events_fwd.hpp"

#include "impl/BindEventTraits.hpp"

#include <concepts>
#include <memory>
#include <type_traits>

namespace erbsland::event {

/// Bind a callback to an explicit event target.
/// @tparam tFunction The handler type; invocations must return void.
/// @param events The non-null target on which the handler will execute.
/// @param callback The handler to copy or move into shared storage. Function pointers and `std::function` objects must
/// not be empty.
/// @return A copyable callable that queues owned arguments; copies share the handler.
/// @throws Any exception from allocating or constructing the handler.
/// @seeref{event-bound-callbacks}
/// @tested{BindEventTest}
template <typename tFunction>
    requires impl::EventBindableFunction<tFunction>
[[nodiscard]] auto bindEvent(EventsPtr events, tFunction &&callback);

/// Bind a callback to the event loop currently running on this thread.
/// The calling thread must be running an event loop.
/// @tparam tFunction The handler type; invocations must return void.
/// @param callback The handler to copy or move into shared storage. Function pointers and `std::function` objects must
/// not be empty.
/// @return A copyable callable retaining the current target and sharing its handler with copies.
/// @throws Any exception from allocating or constructing the handler.
/// @seeref{event-bound-callbacks}
/// @tested{BindEventTest}
template <typename tFunction>
    requires impl::EventBindableFunction<tFunction>
[[nodiscard]] auto bindEvent(tFunction &&callback);

/// Bind a retained recipient's method to an explicit event target.
/// @tparam tRecipient The recipient type, optionally const or derived from the method's class.
/// @tparam tMethod The member-function pointer type; invocations must return void.
/// @param events The non-null target on which the method will execute.
/// @param recipient The non-null recipient retained by bindings and queued deliveries.
/// @param method The non-null member-function pointer.
/// @return A copyable callable that queues owned arguments for the retained recipient.
/// @throws Any exception from allocating the shared handler.
/// @seeref{event-bound-callbacks}
/// @tested{BindEventTest}
template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindEvent(EventsPtr events, std::shared_ptr<tRecipient> recipient, tMethod method);

/// Bind a retained recipient's method to the current event loop.
/// The calling thread must be running an event loop.
/// @tparam tRecipient The recipient type, optionally const or derived from the method's class.
/// @tparam tMethod The member-function pointer type; invocations must return void.
/// @param recipient The non-null recipient retained by bindings and queued deliveries.
/// @param method The non-null member-function pointer.
/// @return A copyable callable retaining the current target and the recipient.
/// @throws Any exception from allocating the shared handler.
/// @seeref{event-bound-callbacks}
/// @tested{BindEventTest}
template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindEvent(std::shared_ptr<tRecipient> recipient, tMethod method);

/// Bind a weak recipient's method to an explicit event target.
/// @tparam tRecipient The recipient type, optionally const or derived from the method's class.
/// @tparam tMethod The member-function pointer type; invocations must return void.
/// @param events The non-null target on which the method will execute.
/// @param weakRecipient The recipient to lock during dispatch; empty and expired pointers are allowed.
/// @param method The non-null member-function pointer.
/// @return A copyable callable that queues owned arguments and skips expired recipients at dispatch.
/// @throws Any exception from allocating the shared handler.
/// @seeref{event-bound-callbacks}
/// @tested{BindWeakEventTest}
template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindWeakEvent(EventsPtr events, std::weak_ptr<tRecipient> weakRecipient, tMethod method);

/// Bind a weak recipient's method to the current event loop.
/// The calling thread must be running an event loop.
/// @tparam tRecipient The recipient type, optionally const or derived from the method's class.
/// @tparam tMethod The member-function pointer type; invocations must return void.
/// @param weakRecipient The recipient to lock during dispatch; empty and expired pointers are allowed.
/// @param method The non-null member-function pointer.
/// @return A copyable callable retaining the current target and checking recipient lifetime at dispatch.
/// @throws Any exception from allocating the shared handler.
/// @seeref{event-bound-callbacks}
/// @tested{BindWeakEventTest}
template <typename tRecipient, typename tMethod>
    requires std::is_member_function_pointer_v<tMethod>
[[nodiscard]] auto bindWeakEvent(std::weak_ptr<tRecipient> weakRecipient, tMethod method);

}

#include "BindEvent.tpp"
