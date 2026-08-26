// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BuiltInFilters.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"

namespace erbsland::text::render::impl::built_in_filters {

using namespace text::literals;

auto find(const String &name) noexcept -> std::optional<BuiltInFilter> {
    switch (name.length().toSizeT()) {
    case 1U:
        if (name == "d"_el) {
            return BuiltInFilter::Default;
        }
        return {};
    case 3U:
        if (name == "abs"_el) {
            return BuiltInFilter::Absolute;
        }
        if (name == "sum"_el) {
            return BuiltInFilter::Sum;
        }
        if (name == "min"_el) {
            return BuiltInFilter::Minimum;
        }
        if (name == "max"_el) {
            return BuiltInFilter::Maximum;
        }
        return {};
    case 4U:
        if (name == "trim"_el) {
            return BuiltInFilter::Trim;
        }
        if (name == "last"_el) {
            return BuiltInFilter::Last;
        }
        if (name == "join"_el) {
            return BuiltInFilter::Join;
        }
        if (name == "sort"_el) {
            return BuiltInFilter::Sort;
        }
        if (name == "keys"_el) {
            return BuiltInFilter::Keys;
        }
        return {};
    case 5U:
        if (name == "lower"_el) {
            return BuiltInFilter::Lower;
        }
        if (name == "upper"_el) {
            return BuiltInFilter::Upper;
        }
        if (name == "first"_el) {
            return BuiltInFilter::First;
        }
        if (name == "count"_el) {
            return BuiltInFilter::Length;
        }
        if (name == "items"_el) {
            return BuiltInFilter::Items;
        }
        if (name == "round"_el) {
            return BuiltInFilter::Round;
        }
        return {};
    case 6U:
        if (name == "length"_el) {
            return BuiltInFilter::Length;
        }
        if (name == "values"_el) {
            return BuiltInFilter::Values;
        }
        if (name == "tojson"_el) {
            return BuiltInFilter::ToJson;
        }
        return {};
    case 7U:
        if (name == "replace"_el) {
            return BuiltInFilter::Replace;
        }
        if (name == "reverse"_el) {
            return BuiltInFilter::Reverse;
        }
        if (name == "default"_el) {
            return BuiltInFilter::Default;
        }
        return {};
    case 10U:
        if (name == "capitalize"_el) {
            return BuiltInFilter::Capitalize;
        }
        return {};
    default:
        return {};
    }
}
}
