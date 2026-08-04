// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::random {

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
auto Random::buildIntegerList(const unit::ItemCount count, const T minimum, const T maximum) -> util::List<T> {
    if (count.isZero() || count.isInfinite()) {
        return {};
    }
    auto result = util::List<T>{};
    result.reserve(count);
    for (auto i = unit::ItemCount{}; i < count; ++i) {
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
auto Random::buildElementList(const unit::ItemCount count, const std::span<const T> choices) -> util::List<T> {
    if (count.isZero() || count.isInfinite() || choices.empty()) {
        return {};
    }
    auto result = util::List<T>{};
    result.reserve(count);
    for (auto i = unit::ItemCount{}; i < count; ++i) {
        result.append(selectElement(choices));
    }
    return result;
}

template <typename T, typename Self>
auto Random::buildElementList(const unit::ItemCount count, const util::List<T, Self> &choices) ->
    typename util::List<T, Self>::Self {
    using Result = typename util::List<T, Self>::Self;
    if (count.isZero() || count.isInfinite() || choices.count().isZero()) {
        return {};
    }
    auto result = Result{};
    result.reserve(count);
    for (auto i = unit::ItemCount{}; i < count; ++i) {
        result.append(selectElement(choices));
    }
    return result;
}

template <typename T, typename Compare, typename Self>
auto Random::buildElementList(const unit::ItemCount count, const util::Set<T, Compare, Self> &choices)
    -> util::List<T> {
    return buildElementList(count, choices.toList());
}

template <typename T, typename Hash, typename Equal, typename Self>
auto Random::buildElementList(const unit::ItemCount count, const util::HashSet<T, Hash, Equal, Self> &choices)
    -> util::List<T> {
    return buildElementList(count, choices.toList());
}

template <typename T>
auto Random::buildUniqueElementList(const unit::ItemCount count, const std::span<const T> choices) -> util::List<T> {
    if (count.isZero() || count.isInfinite() || choices.empty()) {
        return {};
    }
    auto values = std::vector<T>{choices.begin(), choices.end()};
    shuffle(values);
    const auto resultCount = std::min(count.toSizeT(), values.size());
    auto result = util::List<T>{};
    result.reserve(unit::ItemCount::fromSizeT(resultCount));
    for (auto i = std::size_t{0}; i < resultCount; ++i) {
        result.append(values[i]);
    }
    return result;
}

template <typename T, typename Self>
auto Random::buildUniqueElementList(const unit::ItemCount count, const util::List<T, Self> &choices) ->
    typename util::List<T, Self>::Self {
    using Result = typename util::List<T, Self>::Self;
    if (count.isZero() || count.isInfinite() || choices.count().isZero()) {
        return {};
    }
    auto values = choices.toStdVector();
    shuffle(values);
    const auto resultCount = std::min(count.toSizeT(), values.size());
    auto result = Result{};
    result.reserve(unit::ItemCount::fromSizeT(resultCount));
    for (auto i = std::size_t{0}; i < resultCount; ++i) {
        result.append(values[i]);
    }
    return result;
}

template <typename T, typename Compare, typename Self>
auto Random::buildUniqueElementList(const unit::ItemCount count, const util::Set<T, Compare, Self> &choices)
    -> util::List<T> {
    return buildUniqueElementList(count, choices.toList());
}

template <typename T, typename Hash, typename Equal, typename Self>
auto Random::buildUniqueElementList(const unit::ItemCount count, const util::HashSet<T, Hash, Equal, Self> &choices)
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
