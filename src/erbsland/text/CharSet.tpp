// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/impl/LoopControl.hpp"

#include <concepts>
#include <functional>
#include <initializer_list>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace erbsland::text {

template <typename Function>
auto CharSet::forEach(Function function) const -> util::LoopResult {
    if constexpr (std::invocable<Function &, const CharRange &>) {
        for (const auto &range : ranges()) {
            const auto status = processForEach(function, range);
            if (status != util::LoopStatus::Continue) {
                return util::impl::loopStatusToResult(status);
            }
        }
        return util::LoopResult::Success;
    } else if constexpr (std::invocable<Function &, Char>) {
        for (const auto &range : ranges()) {
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

    auto resultRanges = Ranges{};
    auto rangeStart = std::optional<Char>{};
    auto rangeEnd = std::optional<Char>{};
    const auto flushRange = [&]() -> void {
        if (rangeStart.has_value() && rangeEnd.has_value()) {
            addTo(resultRanges, CharRange{*rangeStart, *rangeEnd});
            rangeStart.reset();
            rangeEnd.reset();
        }
    };

    forEach([&](const Char character) -> util::LoopStatus {
        const auto mappedCharacter = static_cast<Char>(std::invoke(function, character));
        if (!mappedCharacter.isValidUnicode()) {
            return util::LoopStatus::Continue;
        }
        if (!rangeStart.has_value()) {
            rangeStart = mappedCharacter;
            rangeEnd = mappedCharacter;
            return util::LoopStatus::Continue;
        }
        if (mappedCharacter == *rangeEnd) {
            return util::LoopStatus::Continue;
        }
        const auto nextCharacter = nextScalar(*rangeEnd);
        if (nextCharacter.has_value() && mappedCharacter == *nextCharacter) {
            rangeEnd = mappedCharacter;
            return util::LoopStatus::Continue;
        }
        flushRange();
        rangeStart = mappedCharacter;
        rangeEnd = mappedCharacter;
        return util::LoopStatus::Continue;
    });
    flushRange();
    auto result = CharSet{};
    result.assign(std::move(resultRanges));
    return result;
}

template <typename Function, typename Value>
auto CharSet::processForEach(Function &function, Value &&value) -> util::LoopStatus {
    return util::impl::invokeLoopFunction(function, std::forward<Value>(value));
}

}
