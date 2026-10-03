// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8String.hpp"

#include "impl/U8StringModifyTools.hpp"

#include <utility>

namespace erbsland::text {

using namespace impl;
using namespace unit;

auto U8String::removed(const ByteRange range) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive()}.removedIfChanged(range)) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::removed(const CpRange range) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive()}.removedIfChanged(range)) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::removedAll(const CharSet &characters) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive()}.removedAllIfChanged(characters)) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::removedAll(const U8String &text, const CharCompareFn compareFn) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive()}.removedAllIfChanged(text.dataView(), compareFn)) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::removedFirst(const U8String &text, const CharCompareFn compareFn) const -> U8String {
    if (auto result =
            U8StringModifyTools{dataView(), isSensitive()}.removedFirstIfChanged(text.dataView(), compareFn)) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::kept(const ByteRange range) const -> U8String {
    return U8String{U8StringModifyTools{dataView(), isSensitive()}.kept(range)};
}

auto U8String::kept(const CpRange range) const -> U8String {
    return U8String{U8StringModifyTools{dataView(), isSensitive()}.kept(range)};
}

auto U8String::inserted(const ByteIndex index, const U8String &text) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive() || text.isSensitive()}.insertedIfChanged(
            index, text.dataView())) {
        return U8String{std::move(result.value())};
    }
    auto result = *this;
    if (text.isSensitive()) {
        result.markAsSensitive();
    }
    return result;
}

auto U8String::inserted(const CpIndex index, const U8String &text) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive() || text.isSensitive()}.insertedIfChanged(
            index, text.dataView())) {
        return U8String{std::move(result.value())};
    }
    auto result = *this;
    if (text.isSensitive()) {
        result.markAsSensitive();
    }
    return result;
}

auto U8String::replaced(const ByteRange range, const U8String &text) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive() || text.isSensitive()}.replacedIfChanged(
            range, text.dataView())) {
        return U8String{std::move(result.value())};
    }
    auto result = *this;
    if (text.isSensitive()) {
        result.markAsSensitive();
    }
    return result;
}

auto U8String::replaced(const CpRange range, const U8String &text) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive() || text.isSensitive()}.replacedIfChanged(
            range, text.dataView())) {
        return U8String{std::move(result.value())};
    }
    auto result = *this;
    if (text.isSensitive()) {
        result.markAsSensitive();
    }
    return result;
}

auto U8String::replacedAll(const CharSet &characters, const Char replacement) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive()}.replacedAllIfChanged(characters, replacement)) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::replacedAll(const CharSet &characters, const U8String &replacement) const -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive() || replacement.isSensitive()}.replacedAllIfChanged(
            characters, replacement.dataView())) {
        return U8String{std::move(result.value())};
    }
    auto result = *this;
    if (replacement.isSensitive()) {
        result.markAsSensitive();
    }
    return result;
}

auto U8String::replacedAll(const U8String &text, const U8String &replacement, const CharCompareFn compareFn) const
    -> U8String {
    if (auto result = U8StringModifyTools{dataView(), isSensitive() || replacement.isSensitive()}.replacedAllIfChanged(
            text.dataView(), replacement.dataView(), compareFn)) {
        return U8String{std::move(result.value())};
    }
    auto result = *this;
    if (replacement.isSensitive()) {
        result.markAsSensitive();
    }
    return result;
}

auto U8String::replacedFirst(const U8String &text, const U8String &replacement, const CharCompareFn compareFn) const
    -> U8String {
    if (auto result =
            U8StringModifyTools{dataView(), isSensitive() || replacement.isSensitive()}.replacedFirstIfChanged(
                text.dataView(), replacement.dataView(), compareFn)) {
        return U8String{std::move(result.value())};
    }
    auto result = *this;
    if (replacement.isSensitive()) {
        result.markAsSensitive();
    }
    return result;
}

}
