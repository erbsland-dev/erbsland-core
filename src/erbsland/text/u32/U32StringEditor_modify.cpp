// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringEditor.hpp"

#include "U32String.hpp"

#include "impl/U32StringModifyTools.hpp"

#include <utility>

namespace erbsland::text {

using namespace impl;
using namespace unit;

auto U32StringEditor::removed(const CpRange range) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.removedIfChanged(range)) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::removedAll(const CharSet &characters) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.removedAllIfChanged(characters)) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::removedAll(const U32String &text, const CharCompareFn compareFn) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.removedAllIfChanged(text.dataView(), compareFn)) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::removedFirst(const U32String &text, const CharCompareFn compareFn) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.removedFirstIfChanged(text.dataView(), compareFn)) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::kept(const CpRange range) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.kept(range)};
}

auto U32StringEditor::inserted(const CpIndex index, const U32String &text) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.insertedIfChanged(index, text.dataView())) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::replaced(const CpRange range, const U32String &text) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.replacedIfChanged(range, text.dataView())) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::replacedFirst(
    const U32String &text, const U32String &replacement, const CharCompareFn compareFn) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.replacedFirstIfChanged(
            text.dataView(), replacement.dataView(), compareFn)) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::replacedAll(const CharSet &characters, const Char replacement) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.replacedAllIfChanged(characters, replacement)) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::replacedAll(const CharSet &characters, const U32String &replacement) const -> U32StringEditor {
    if (auto result = U32StringModifyTools{dataView()}.replacedAllIfChanged(characters, replacement.dataView())) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U32StringEditor::replacedAll(
    const U32String &text, const U32String &replacement, const CharCompareFn compareFn) const -> U32StringEditor {
    if (auto result =
            U32StringModifyTools{dataView()}.replacedAllIfChanged(text.dataView(), replacement.dataView(), compareFn)) {
        return U32StringEditor{std::move(result.value())};
    }
    return *this;
}

}
