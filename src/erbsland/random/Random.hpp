// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../math/IntegerMath.hpp"
#include "../math/IntegerRange.hpp"
#include "../math/IntegerTypes.hpp"
#include "../mem/ByteBlock.hpp"
#include "../text/CharSet.hpp"
#include "../text/String.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/ElementCount.hpp"
#include "../unit/ElementIndex.hpp"
#include "../util/HashSet.hpp"
#include "../util/List.hpp"
#include "../util/Set.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <vector>

namespace erbsland::random {

class Random;
using RandomPtr = std::unique_ptr<Random>;

/// The common interface for random number generators.
/// @seedoc{/topics/random/overview}
/// @tested{RandomTest}
class Random {
public:
    /// Destroy this random generator.
    virtual ~Random() = default;

    // defaults
    Random(const Random &) = delete;
    auto operator=(const Random &) -> Random & = delete;

public:
    /// Create a random native integer in the inclusive range.
    /// @tparam T The native integer result type.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    template <math::NativeInteger T>
    [[nodiscard]] auto selectInteger(T minimum, T maximum) -> T;
    /// Create a random native integer in the inclusive range.
    /// @tparam T The native integer result type.
    /// @param range The inclusive integer range.
    /// @return A random integer in the inclusive range.
    template <math::NativeInteger T>
    [[nodiscard]] auto selectInteger(math::IntegerRange<T> range) -> T;
    /// Create a list of random native integers.
    /// @tparam T The native integer result type.
    /// @param count The number of values to build. Zero or infinite counts return an empty list.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A list with `count` random integers, or an empty list.
    template <math::NativeInteger T>
    [[nodiscard]] auto buildIntegerList(unit::ElementCount count, T minimum, T maximum) -> util::List<T>;
    /// Create a random UTF-8 string with characters from the given set.
    /// @param length The number of Unicode code points to build. Zero or infinite lengths return an empty string.
    /// @param characters The character choices. An empty set returns an empty string.
    /// @return A random UTF-8 string, or an empty string.
    [[nodiscard]] auto buildString(unit::CpLength length, const text::CharSet &characters) -> text::String;
    /// Create a block of random bytes.
    /// @param length The number of bytes to build. Zero or infinite lengths return an empty block.
    /// @return A block with random bytes, or an empty block.
    [[nodiscard]] auto buildByteBlock(unit::ByteLength length) -> mem::ByteBlock;
    /// Select a valid element index for a container with `count` elements.
    /// @param count The number of available elements.
    /// @return A random index in `[0, count)`, or `ElementIndex::noIndex()` for zero or infinite counts.
    [[nodiscard]] auto selectIndex(unit::ElementCount count) -> unit::ElementIndex;

public: // element selection
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T>
    [[nodiscard]] auto selectElement(std::span<const T> choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T>
    [[nodiscard]] auto selectElement(const std::vector<T> &choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T>
    [[nodiscard]] auto selectElement(std::initializer_list<T> choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @tparam Self The list CRTP type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T, typename Self>
    [[nodiscard]] auto selectElement(const util::List<T, Self> &choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @tparam Compare The ordered-set comparison type.
    /// @tparam Self The ordered-set CRTP type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T, typename Compare, typename Self>
    [[nodiscard]] auto selectElement(const util::Set<T, Compare, Self> &choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @tparam Hash The hash-set hash type.
    /// @tparam Equal The hash-set equality type.
    /// @tparam Self The hash-set CRTP type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T, typename Hash, typename Equal, typename Self>
    [[nodiscard]] auto selectElement(const util::HashSet<T, Hash, Equal, Self> &choices, const T &valueIfEmpty = {})
        -> T;
    /// Build a list by sampling elements with replacement.
    /// @tparam T The element type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with sampled elements, or an empty list.
    template <typename T>
    [[nodiscard]] auto buildElementList(unit::ElementCount count, std::span<const T> choices) -> util::List<T>;
    /// Build a list by sampling elements with replacement.
    /// @tparam T The element type.
    /// @tparam Self The list CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A matching list with sampled elements, or an empty list.
    template <typename T, typename Self>
    [[nodiscard]] auto buildElementList(unit::ElementCount count, const util::List<T, Self> &choices) ->
        typename util::List<T, Self>::Self;
    /// Build a list by sampling elements with replacement.
    /// @tparam T The element type.
    /// @tparam Compare The ordered-set comparison type.
    /// @tparam Self The ordered-set CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with sampled elements, or an empty list.
    template <typename T, typename Compare, typename Self>
    [[nodiscard]] auto buildElementList(unit::ElementCount count, const util::Set<T, Compare, Self> &choices)
        -> util::List<T>;
    /// Build a list by sampling elements with replacement.
    /// @tparam T The element type.
    /// @tparam Hash The hash-set hash type.
    /// @tparam Equal The hash-set equality type.
    /// @tparam Self The hash-set CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with sampled elements, or an empty list.
    template <typename T, typename Hash, typename Equal, typename Self>
    [[nodiscard]] auto buildElementList(unit::ElementCount count, const util::HashSet<T, Hash, Equal, Self> &choices)
        -> util::List<T>;
    /// Build a list by sampling elements without replacement.
    /// @tparam T The element type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with unique sampled elements. The list is capped to the number of choices.
    template <typename T>
    [[nodiscard]] auto buildUniqueElementList(unit::ElementCount count, std::span<const T> choices) -> util::List<T>;
    /// Build a list by sampling elements without replacement.
    /// @tparam T The element type.
    /// @tparam Self The list CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A matching list with unique sampled elements. The list is capped to the number of choices.
    template <typename T, typename Self>
    [[nodiscard]] auto buildUniqueElementList(unit::ElementCount count, const util::List<T, Self> &choices) ->
        typename util::List<T, Self>::Self;
    /// Build a list by sampling elements without replacement.
    /// @tparam T The element type.
    /// @tparam Compare The ordered-set comparison type.
    /// @tparam Self The ordered-set CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with unique sampled elements. The list is capped to the number of choices.
    template <typename T, typename Compare, typename Self>
    [[nodiscard]] auto buildUniqueElementList(unit::ElementCount count, const util::Set<T, Compare, Self> &choices)
        -> util::List<T>;
    /// Build a list by sampling elements without replacement.
    /// @tparam T The element type.
    /// @tparam Hash The hash-set hash type.
    /// @tparam Equal The hash-set equality type.
    /// @tparam Self The hash-set CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with unique sampled elements. The list is capped to the number of choices.
    template <typename T, typename Hash, typename Equal, typename Self>
    [[nodiscard]] auto buildUniqueElementList(
        unit::ElementCount count, const util::HashSet<T, Hash, Equal, Self> &choices) -> util::List<T>;

public: // shuffling
    /// Shuffle a random-access span in place.
    /// @tparam T The element type.
    /// @param values The values to shuffle. Empty and single-element spans are unchanged.
    template <typename T>
    void shuffle(std::span<T> values);
    /// Shuffle a vector in place.
    /// @tparam T The element type.
    /// @param values The values to shuffle. Empty and single-element vectors are unchanged.
    template <typename T>
    void shuffle(std::vector<T> &values);
    /// Shuffle a list in place.
    /// @tparam T The element type.
    /// @tparam Self The list CRTP type.
    /// @param values The values to shuffle. Empty and single-element lists are unchanged.
    template <typename T, typename Self>
    void shuffle(util::List<T, Self> &values);

public: // source methods
    /// Generate a random 32-bit signed integer in the inclusive range.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    [[nodiscard]] virtual auto getInt32(int32_t minimum, int32_t maximum) -> int32_t = 0;
    /// Generate a random 32-bit unsigned integer in the inclusive range.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    [[nodiscard]] virtual auto getUInt32(uint32_t minimum, uint32_t maximum) -> uint32_t = 0;
    /// Generate a random 64-bit signed integer in the inclusive range.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    [[nodiscard]] virtual auto getInt64(int64_t minimum, int64_t maximum) -> int64_t = 0;
    /// Generate a random 64-bit unsigned integer in the inclusive range.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    [[nodiscard]] virtual auto getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t = 0;
    /// Generate a random floating-point value.
    /// @param minimum The lower bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper bound. Reversed bounds are ordered automatically.
    /// @return A random floating-point value in the range supported by the generator.
    [[nodiscard]] virtual auto getDouble(double minimum, double maximum) -> double = 0;
    /// Generate a random boolean value.
    /// @return A random boolean value.
    [[nodiscard]] virtual auto getBool() -> bool = 0;
    /// Fill the destination with random bytes.
    /// @param destination The byte span to fill. An empty span is accepted.
    virtual void fillBytes(std::span<std::byte> destination) = 0;

protected:
    /// Create a random generator base.
    Random() = default;
};

template <math::NativeInteger T>
auto Random::selectInteger(T minimum, T maximum) -> T {
    using Value = std::remove_cv_t<T>;
    if constexpr (sizeof(Value) <= 4U) {
        if constexpr (std::is_signed_v<Value>) {
            return static_cast<T>(getInt32(static_cast<int32_t>(minimum), static_cast<int32_t>(maximum)));
        } else {
            return static_cast<T>(getUInt32(static_cast<uint32_t>(minimum), static_cast<uint32_t>(maximum)));
        }
    } else {
        if constexpr (std::is_signed_v<Value>) {
            return static_cast<T>(getInt64(static_cast<int64_t>(minimum), static_cast<int64_t>(maximum)));
        } else {
            return static_cast<T>(getUInt64(static_cast<uint64_t>(minimum), static_cast<uint64_t>(maximum)));
        }
    }
}

template <math::NativeInteger T>
auto Random::selectInteger(const math::IntegerRange<T> range) -> T {
    return selectInteger(range.minimum(), range.maximum());
}

template <math::NativeInteger T>
auto Random::buildIntegerList(const unit::ElementCount count, const T minimum, const T maximum) -> util::List<T> {
    if (count.isZero() || count.isInfinite()) {
        return {};
    }
    auto result = util::List<T>{};
    result.reserve(count);
    for (auto i = unit::ElementCount{}; i < count; ++i) {
        result.append(selectInteger(minimum, maximum));
    }
    return result;
}

template <typename T>
auto Random::selectElement(const std::span<const T> choices, const T &valueIfEmpty) -> T {
    if (choices.empty()) {
        return valueIfEmpty;
    }
    const auto index = selectInteger<std::size_t>(0U, choices.size() - 1U);
    return choices[index];
}

template <typename T>
auto Random::selectElement(const std::vector<T> &choices, const T &valueIfEmpty) -> T {
    return selectElement(std::span<const T>{choices}, valueIfEmpty);
}

template <typename T>
auto Random::selectElement(const std::initializer_list<T> choices, const T &valueIfEmpty) -> T {
    return selectElement(std::span<const T>{choices.begin(), choices.size()}, valueIfEmpty);
}

template <typename T, typename Self>
auto Random::selectElement(const util::List<T, Self> &choices, const T &valueIfEmpty) -> T {
    if (choices.count().isZero()) {
        return valueIfEmpty;
    }
    const auto index = selectIndex(choices.count());
    return choices.get(index, valueIfEmpty);
}

template <typename T, typename Compare, typename Self>
auto Random::selectElement(const util::Set<T, Compare, Self> &choices, const T &valueIfEmpty) -> T {
    return selectElement(choices.toList(), valueIfEmpty);
}

template <typename T, typename Hash, typename Equal, typename Self>
auto Random::selectElement(const util::HashSet<T, Hash, Equal, Self> &choices, const T &valueIfEmpty) -> T {
    return selectElement(choices.toList(), valueIfEmpty);
}

template <typename T>
auto Random::buildElementList(const unit::ElementCount count, const std::span<const T> choices) -> util::List<T> {
    if (count.isZero() || count.isInfinite() || choices.empty()) {
        return {};
    }
    auto result = util::List<T>{};
    result.reserve(count);
    for (auto i = unit::ElementCount{}; i < count; ++i) {
        result.append(selectElement(choices));
    }
    return result;
}

template <typename T, typename Self>
auto Random::buildElementList(const unit::ElementCount count, const util::List<T, Self> &choices) ->
    typename util::List<T, Self>::Self {
    using Result = typename util::List<T, Self>::Self;
    if (count.isZero() || count.isInfinite() || choices.count().isZero()) {
        return {};
    }
    auto result = Result{};
    result.reserve(count);
    for (auto i = unit::ElementCount{}; i < count; ++i) {
        result.append(selectElement(choices));
    }
    return result;
}

template <typename T, typename Compare, typename Self>
auto Random::buildElementList(const unit::ElementCount count, const util::Set<T, Compare, Self> &choices)
    -> util::List<T> {
    return buildElementList(count, choices.toList());
}

template <typename T, typename Hash, typename Equal, typename Self>
auto Random::buildElementList(const unit::ElementCount count, const util::HashSet<T, Hash, Equal, Self> &choices)
    -> util::List<T> {
    return buildElementList(count, choices.toList());
}

template <typename T>
auto Random::buildUniqueElementList(const unit::ElementCount count, const std::span<const T> choices) -> util::List<T> {
    if (count.isZero() || count.isInfinite() || choices.empty()) {
        return {};
    }
    auto values = std::vector<T>{choices.begin(), choices.end()};
    shuffle(values);
    const auto resultCount = std::min(count.toSizeT(), values.size());
    auto result = util::List<T>{};
    result.reserve(unit::ElementCount::fromSizeT(resultCount));
    for (auto i = std::size_t{0}; i < resultCount; ++i) {
        result.append(values[i]);
    }
    return result;
}

template <typename T, typename Self>
auto Random::buildUniqueElementList(const unit::ElementCount count, const util::List<T, Self> &choices) ->
    typename util::List<T, Self>::Self {
    using Result = typename util::List<T, Self>::Self;
    if (count.isZero() || count.isInfinite() || choices.count().isZero()) {
        return {};
    }
    auto values = choices.toStdVector();
    shuffle(values);
    const auto resultCount = std::min(count.toSizeT(), values.size());
    auto result = Result{};
    result.reserve(unit::ElementCount::fromSizeT(resultCount));
    for (auto i = std::size_t{0}; i < resultCount; ++i) {
        result.append(values[i]);
    }
    return result;
}

template <typename T, typename Compare, typename Self>
auto Random::buildUniqueElementList(const unit::ElementCount count, const util::Set<T, Compare, Self> &choices)
    -> util::List<T> {
    return buildUniqueElementList(count, choices.toList());
}

/// @copydoc buildUniqueElementList(unit::ElementCount, const util::HashSet<T, Hash, Equal, Self> &)
template <typename T, typename Hash, typename Equal, typename Self>
auto Random::buildUniqueElementList(const unit::ElementCount count, const util::HashSet<T, Hash, Equal, Self> &choices)
    -> util::List<T> {
    return buildUniqueElementList(count, choices.toList());
}

template <typename T>
void Random::shuffle(const std::span<T> values) {
    for (auto i = values.size(); i > 1U; --i) {
        using std::swap;
        swap(values[i - 1U], values[selectInteger<std::size_t>(0U, i - 1U)]);
    }
}

template <typename T>
void Random::shuffle(std::vector<T> &values) {
    shuffle(std::span<T>{values});
}

template <typename T, typename Self>
void Random::shuffle(util::List<T, Self> &values) {
    auto raw = values.toStdVector();
    shuffle(raw);
    values = util::List<T, Self>{std::move(raw)};
}

}
