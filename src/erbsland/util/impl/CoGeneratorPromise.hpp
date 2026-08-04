// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoGeneratorPromise_fwd.hpp"

#include "../CoGenerator_fwd.hpp"

#include <concepts>
#include <coroutine>
#include <exception>
#include <optional>
#include <utility>

namespace erbsland::util::impl {

/// Store yielded values and errors for one generator coroutine.
template <typename tValue>
class CoGeneratorPromise {
    friend class CoGenerator<tValue>;
    friend class CoGeneratorIterator<tValue>;

public:
    using Value = tValue;

    /// Create the coroutine return object.
    [[nodiscard]] auto get_return_object() -> CoGenerator<tValue>;
    /// Suspend before the first coroutine statement.
    [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_always { return {}; }
    /// Suspend after the coroutine has finished so the owner can destroy the frame.
    [[nodiscard]] auto final_suspend() noexcept -> std::suspend_always { return {}; }
    /// Store a yielded value in the coroutine promise.
    template <typename tYielded>
        requires std::constructible_from<Value, tYielded>
    auto yield_value(tYielded &&value) -> std::suspend_always {
        _currentValue.emplace(std::forward<tYielded>(value));
        return {};
    }
    /// Finish the coroutine without a final value.
    void return_void() noexcept {}
    /// Store an unhandled coroutine exception until the consumer resumes or accesses the generator.
    void unhandled_exception() noexcept { _exception = std::current_exception(); }

private:
    std::optional<Value> _currentValue{};
    std::exception_ptr _exception{};
};

}
