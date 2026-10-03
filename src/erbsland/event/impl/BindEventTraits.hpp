// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

namespace erbsland::event::impl {

/// Identify standard function wrappers for empty-handler validation.
/// @tparam T The decayed handler type.
/// @tested{BindEventTest}
template <typename T>
struct IsEventStdFunction : std::false_type {};

/// Identify a standard function wrapper with any signature.
/// @tparam tResult The function result.
/// @tparam tArguments The function arguments.
/// @tested{BindEventTest}
template <typename tResult, typename... tArguments>
struct IsEventStdFunction<std::function<tResult(tArguments...)>> : std::true_type {};

/// Require a callback that can be stored without binding a member pointer directly.
/// @tparam T The callback expression type, preserving lvalue references.
template <typename T>
concept EventBindableFunction =
    (!std::is_member_pointer_v<std::decay_t<T>>) && std::constructible_from<std::decay_t<T>, T>;

/// Require an invocation with an exact void result.
/// @tparam tFunction The handler reference type.
/// @tparam tArguments The argument expression types.
template <typename tFunction, typename... tArguments>
concept EventVoidInvocable =
    std::invocable<tFunction, tArguments...> && std::same_as<std::invoke_result_t<tFunction, tArguments...>, void>;

}
