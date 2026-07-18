// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "Match.hpp"
#include "Match16.hpp"
#include "Match32.hpp"

#include "impl/input/StreamInput.hpp"
#include "impl/input/StringInput.hpp"
#include "impl/input/U16StringInput.hpp"
#include "impl/input/U32StringInput.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::re {

using namespace text::literals;

auto RegEx::collectAll(const text::String &text) const -> MatchList {
    return collectAll(impl::StringInput::create(text));
}

auto RegEx::collectAll(const text::U16String &text) const -> Match16List {
    return collectAll(impl::U16StringInput::create(text));
}

auto RegEx::collectAll(const text::U32String &text) const -> Match32List {
    return collectAll(impl::U32StringInput::create(text));
}

auto RegEx::collectAll(const stream::TextInputStreamPtr &input) const -> MatchList {
    return collectAll(impl::StreamInput::create(input));
}

auto RegEx::collectAll(const InputPtr &input) const -> MatchList {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto result = MatchList{};
    for (auto match : findAll(input)) {
        result.emplace_back(std::move(match));
    }
    return result;
}

auto RegEx::collectAll(const Input16Ptr &input) const -> Match16List {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto result = Match16List{};
    for (auto match : findAll(input)) {
        result.emplace_back(std::move(match));
    }
    return result;
}

auto RegEx::collectAll(const Input32Ptr &input) const -> Match32List {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto result = Match32List{};
    for (auto match : findAll(input)) {
        result.emplace_back(std::move(match));
    }
    return result;
}

}
