// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16String.hpp"

#include "impl/U16StringModifyTools.hpp"

#include <utility>

namespace erbsland::text {

using namespace impl;
using namespace unit;

auto U16String::removed(const U16Range range) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.removedIfChanged(range)) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::removed(const CpRange range) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.removedIfChanged(range)) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::removedAll(const CharSet &characters) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.removedAllIfChanged(characters)) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::removedAll(const U16String &text, const CharCompareFn compareFn) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.removedAllIfChanged(text.dataView(), compareFn)) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::removedFirst(const U16String &text, const CharCompareFn compareFn) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.removedFirstIfChanged(text.dataView(), compareFn)) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::kept(const U16Range range) const -> U16String {
    return U16String{U16StringModifyTools{dataView()}.kept(range)};
}

auto U16String::kept(const CpRange range) const -> U16String {
    return U16String{U16StringModifyTools{dataView()}.kept(range)};
}

auto U16String::inserted(const U16Index index, const U16String &text) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.insertedIfChanged(index, text.dataView())) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::inserted(const CpIndex index, const U16String &text) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.insertedIfChanged(index, text.dataView())) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::replaced(const U16Range range, const U16String &text) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.replacedIfChanged(range, text.dataView())) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::replaced(const CpRange range, const U16String &text) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.replacedIfChanged(range, text.dataView())) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::replacedAll(const CharSet &characters, const Char replacement) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.replacedAllIfChanged(characters, replacement)) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::replacedAll(const CharSet &characters, const U16String &replacement) const -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.replacedAllIfChanged(characters, replacement.dataView())) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::replacedAll(const U16String &text, const U16String &replacement, const CharCompareFn compareFn) const
    -> U16String {
    if (auto result =
            U16StringModifyTools{dataView()}.replacedAllIfChanged(text.dataView(), replacement.dataView(), compareFn)) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

auto U16String::replacedFirst(const U16String &text, const U16String &replacement, const CharCompareFn compareFn) const
    -> U16String {
    if (auto result = U16StringModifyTools{dataView()}.replacedFirstIfChanged(
            text.dataView(), replacement.dataView(), compareFn)) {
        return U16String{std::move(result.value())};
    }
    return *this;
}

}
