// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfErrorCategory.hpp"

#include "../text/Literals.hpp"

#include <cassert>

namespace erbsland::conf {

using namespace text::literals;

ConfErrorCategory::ValueMap ConfErrorCategory::_valueMap = {
    std::make_tuple(IO, 1, text::String{"IO"_el}),
    std::make_tuple(Encoding, 2, text::String{"Encoding"_el}),
    std::make_tuple(UnexpectedEnd, 3, text::String{"UnexpectedEnd"_el}),
    std::make_tuple(Character, 4, text::String{"Character"_el}),
    std::make_tuple(Syntax, 5, text::String{"Syntax"_el}),
    std::make_tuple(LimitExceeded, 6, text::String{"LimitExceeded"_el}),
    std::make_tuple(NameConflict, 7, text::String{"NameConflict"_el}),
    std::make_tuple(Indentation, 8, text::String{"Indentation"_el}),
    std::make_tuple(Unsupported, 9, text::String{"Unsupported"_el}),
    std::make_tuple(Signature, 10, text::String{"Signature"_el}),
    std::make_tuple(Access, 11, text::String{"Access"_el}),
    std::make_tuple(Validation, 12, text::String{"Validation"_el}),
    std::make_tuple(Internal, 99, text::String{"Internal"_el}),
    // Additional categories for the API of this parser.
    std::make_tuple(ValueNotFound, 101, text::String{"ValueNotFound"_el}),
    std::make_tuple(TypeMismatch, 102, text::String{"TypeMismatch"_el}),
};

auto ConfErrorCategory::toText() const noexcept -> text::String {
    auto it = std::ranges::find_if(
        _valueMap, [this](const ValueEntry &entry) -> bool { return std::get<0>(entry) == _value; });
    assert(it != _valueMap.end());
    return std::get<2>(*it);
}

auto ConfErrorCategory::toCode() const noexcept -> int {
    auto it = std::ranges::find_if(
        _valueMap, [this](const ValueEntry &entry) -> bool { return std::get<0>(entry) == _value; });
    assert(it != _valueMap.end());
    return std::get<1>(*it);
}

}
