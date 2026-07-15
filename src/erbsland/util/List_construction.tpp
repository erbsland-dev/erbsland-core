// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tElement, typename tSelf>
List<tElement, tSelf>::List() : _storage{} {
}

template <typename tElement, typename tSelf>
List<tElement, tSelf>::List(std::initializer_list<Element> values) : _storage{Storage::from(Raw{values})} {
}

template <typename tElement, typename tSelf>
List<tElement, tSelf>::List(Element value) {
    auto raw = Raw{};
    raw.push_back(std::move(value));
    _storage.setData(std::move(raw));
}

template <typename tElement, typename tSelf>
List<tElement, tSelf>::List(const Count count, const Element &value) :
    _storage{Storage::create(countToSize(count), value)} {
}

template <typename tElement, typename tSelf>
List<tElement, tSelf>::List(const Raw &raw) : _storage{Storage::from(raw)} {
}

template <typename tElement, typename tSelf>
List<tElement, tSelf>::List(Raw &&raw) : _storage{Storage::from(std::move(raw))} {
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator+(const Self &other) const -> Self {
    auto result = self();
    result.append(other);
    return result;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator+(const Element &value) const -> Self {
    auto result = self();
    result.append(value);
    return result;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator+(Element &&value) const -> Self {
    auto result = self();
    result.append(std::move(value));
    return result;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator+=(const Self &other) -> Self & {
    return append(other);
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator+=(const Element &value) -> Self & {
    return append(value);
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator+=(Element &&value) -> Self & {
    return append(std::move(value));
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator[](const Index index) const -> Element {
    return get(index);
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator<=>(const Self &other) const -> std::strong_ordering {
    return compare(other);
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::operator==(const Self &other) const -> bool {
    return raw() == other.raw();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::raw() const noexcept -> const Raw & {
    return _storage.data();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::toRawValue() const noexcept -> const Raw & {
    return raw();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::toStdVector() const -> Raw {
    return raw();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::toStdSet() const -> std::set<Element> {
    const auto &data = raw();
    return {data.begin(), data.end()};
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::mutableRaw() -> Raw & {
    return _storage.detachedData();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::self() noexcept -> Self & {
    return static_cast<Self &>(*this);
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::self() const noexcept -> const Self & {
    return static_cast<const Self &>(*this);
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::makeSelf(Raw raw) -> Self {
    return Self{std::move(raw)};
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::countToSize(const Count count) -> std::size_t {
    return count.toSizeTOrThrow();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::validIndex(const Index index, const std::size_t size) noexcept -> bool {
    return !index.isNoIndex() && index.toSizeT() < size;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::clampedRange(const Range range, const Count bounds) noexcept -> Range {
    return range.clampedTo(bounds);
}

}
