// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InternalView.hpp"

#include "../../../text/EscapeFormat.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::conf::impl {

using namespace text;

InternalView::InternalView(const String &name, const Value &value, PrivateTag) noexcept {
    _values.emplace_back(name, value);
}

auto InternalView::create() noexcept -> InternalViewPtr {
    return std::make_shared<InternalView>(PrivateTag{});
}

auto InternalView::create(const String &name, const Value &value) noexcept -> InternalViewPtr {
    return std::make_shared<InternalView>(name, value, PrivateTag{});
}

void InternalView::removeValue(const String &name) noexcept {
    _values.erase(
        std::ranges::remove_if(_values, [&](const auto &pair) -> auto { return pair.first == name; }).begin(),
        _values.end());
}

void InternalView::setValue(const String &name, const Value &value) noexcept {
    removeValue(name);
    _values.emplace_back(name, value);
}

void InternalView::setValue(const String &name, const String &value) noexcept {
    removeValue(name);
    _values.emplace_back(name, value);
}

void InternalView::setValue(const String &name, const InternalViewPtr &value) noexcept {
    removeValue(name);
    _values.emplace_back(name, value);
}

void InternalView::setUnsafeText(const String &name, const String &text, const String &textIfEmpty) {
    if (text.isEmpty()) {
        if (textIfEmpty.isEmpty()) {
            setValue(name, "<empty>"_el);
            return;
        }
        setValue(name, textIfEmpty);
    } else {
        const auto safeText = text.toEscaped(EscapeFormat::Display)
                                  .truncated(unit::CpCount::fromSizeT(200), TruncateMode::Middle, "…"_el);
        setValue(name, StringFormat{"\"{}\""_el}.build(safeText));
    }
}

void InternalView::setValue(const String &name, const bool value) noexcept {
    setValue(name, String{value ? "true"_el : "false"_el});
}

auto InternalView::toString(const std::size_t indent) const noexcept -> String {
    StringEditor result;
    for (auto const &line : toLines(indent)) {
        result.append(line);
        result.append("\n"_el);
    }
    return result;
}

auto InternalView::toLines(const std::size_t indent) const noexcept -> StringList {
    StringList lines;
    lines.reserve(unit::ItemCount::fromSizeT(_values.size()));
    StringEditor indentEditor;
    indentEditor.append(U' ', unit::CpCount::fromSizeT(indent));
    const String indentString = indentEditor;
    for (auto const &[name, value] : _values) {
        std::visit(
            [&]<typename VisitedType>(VisitedType const &visitedValue) -> void {
                using DecayType = std::decay_t<VisitedType>;
                if constexpr (std::is_same_v<DecayType, InternalViewPtr>) {
                    lines.append(StringFormat{"{}{}:"_el}.build(indentString, name)); // parent heading
                    auto childLines = visitedValue->toLines(indent + 2);              // recurse + flatten
                    lines.append(childLines);
                } else if constexpr (std::is_same_v<DecayType, String>) {
                    lines.append(StringFormat{"{}{}: {}"_el}.build(indentString, name, visitedValue)); // leaf entry
                }
            },
            value);
    }
    return lines;
}

}
