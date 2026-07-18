// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "impl/engine/Engine.hpp"
#include "impl/input/StreamInput.hpp"
#include "impl/input/StringInput.hpp"
#include "impl/input/U16StringInput.hpp"
#include "impl/input/U32StringInput.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::re {

using namespace text::literals;

auto RegEx::fullMatch(const text::String &text) const -> MatchPtr {
    return fullMatch(impl::StringInput::create(text));
}

auto RegEx::fullMatch(const text::U16String &text) const -> Match16Ptr {
    return fullMatch(impl::U16StringInput::create(text));
}

auto RegEx::fullMatch(const text::U32String &text) const -> Match32Ptr {
    return fullMatch(impl::U32StringInput::create(text));
}

auto RegEx::fullMatch(const stream::TextInputStreamPtr &input) const -> MatchPtr {
    return fullMatch(impl::StreamInput::create(input));
}

auto RegEx::fullMatch(const InputPtr &input) const -> MatchPtr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    if (_engine->fullMatch(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    return input->createMatch(state->createCaptureGroups(_engine->captureGroupNames()));
}

auto RegEx::fullMatch(const Input16Ptr &input) const -> Match16Ptr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    if (_engine->fullMatch(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    return input->createMatch(state->createCaptureGroups(_engine->captureGroupNames()));
}

auto RegEx::fullMatch(const Input32Ptr &input) const -> Match32Ptr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    if (_engine->fullMatch(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    return input->createMatch(state->createCaptureGroups(_engine->captureGroupNames()));
}

}
