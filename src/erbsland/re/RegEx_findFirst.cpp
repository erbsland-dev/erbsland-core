// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "impl/input/StreamInput.hpp"
#include "impl/input/StringInput.hpp"
#include "impl/input/U16StringInput.hpp"
#include "impl/input/U32StringInput.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::re {

using namespace text::literals;

auto RegEx::findFirst(const text::StringView &text) const -> MatchPtr {
    return findFirst(impl::StringInput::create(text));
}

auto RegEx::findFirst(const text::U16StringView &text) const -> Match16Ptr {
    return findFirst(impl::U16StringInput::create(text));
}

auto RegEx::findFirst(const text::U32StringView &text) const -> Match32Ptr {
    return findFirst(impl::U32StringInput::create(text));
}

auto RegEx::findFirst(const stream::TextInputStreamPtr &input) const -> MatchPtr {
    return findFirst(impl::StreamInput::create(input));
}

auto RegEx::findFirst(const InputPtr &input) const -> MatchPtr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    if (_engine->findFirst(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    return input->createMatch(shared_from_this(), state->createCaptureGroups(_engine->captureGroupNames()));
}

auto RegEx::findFirst(const Input16Ptr &input) const -> Match16Ptr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    if (_engine->findFirst(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    return input->createMatch(shared_from_this(), state->createCaptureGroups(_engine->captureGroupNames()));
}

auto RegEx::findFirst(const Input32Ptr &input) const -> Match32Ptr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    if (_engine->findFirst(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    return input->createMatch(shared_from_this(), state->createCaptureGroups(_engine->captureGroupNames()));
}

}
