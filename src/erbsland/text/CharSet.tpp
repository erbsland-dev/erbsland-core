// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/impl/LoopControl.hpp"

#include <concepts>
#include <functional>
#include <initializer_list>
#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::text {

template <typename Function>
auto CharSet::forEach(Function function) const -> util::LoopResult {
    if constexpr (std::invocable<Function &, const CharRange &>) {
        for (const auto &range : rangeSpan()) {
            const auto status = processForEach(function, range);
            if (status != util::LoopStatus::Continue) {
                return util::impl::loopStatusToResult(status);
            }
        }
        return util::LoopResult::Success;
    } else if constexpr (std::invocable<Function &, Char>) {
        for (const auto &range : rangeSpan()) {
            auto character = range.from();
            while (character <= range.to()) {
                const auto status = processForEach(function, character);
                if (status != util::LoopStatus::Continue) {
                    return util::impl::loopStatusToResult(status);
                }
                const auto nextCharacter = nextScalar(character);
                if (!nextCharacter.has_value() || *nextCharacter > range.to()) {
                    break;
                }
                character = *nextCharacter;
            }
        }
        return util::LoopResult::Success;
    } else {
        static_assert(
            cIsSupportedForEachFunction<Function>,
            "CharSet::forEach() requires a function that accepts CharRange or Char.");
    }
}

template <typename Function>
auto CharSet::transform(Function function) const -> CharSet {
    static_assert(std::invocable<Function &, Char>, "CharSet::transform() requires a function that accepts Char.");
    static_assert(
        std::convertible_to<std::invoke_result_t<Function &, Char>, Char>,
        "CharSet::transform() requires a function that returns Char.");

    auto result = CharSet{};
    forEach([&](const Char character) -> util::LoopStatus {
        const auto mappedCharacter = static_cast<Char>(std::invoke(function, character));
        result.add(mappedCharacter);
        return util::LoopStatus::Continue;
    });
    return result;
}

template <typename Function, typename Value>
auto CharSet::processForEach(Function &function, Value &&value) -> util::LoopStatus {
    return util::impl::invokeLoopFunction(function, std::forward<Value>(value));
}

}
