// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "impl/engine/Engine.hpp"
#include "impl/error/InternalError.hpp"
#include "impl/input/StreamInput.hpp"
#include "impl/input/StringInput.hpp"
#include "impl/input/U16StringInput.hpp"
#include "impl/input/U32StringInput.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::re {

using namespace text::literals;

auto RegEx::match(const text::String &text) const -> MatchPtr {
    return match(impl::StringInput::create(text));
}

auto RegEx::match(const text::U16String &text) const -> Match16Ptr {
    return match(impl::U16StringInput::create(text));
}

auto RegEx::match(const text::U32String &text) const -> Match32Ptr {
    return match(impl::U32StringInput::create(text));
}

auto RegEx::match(const stream::TextInputStreamPtr &input) const -> MatchPtr {
    return match(impl::StreamInput::create(input));
}

auto RegEx::match(const InputPtr &input) const -> MatchPtr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    const auto engine = this->engine();
    auto state = engine->createState(input);
    if (engine->match(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    auto captureGroups = state->createCaptureGroups(engine->captureGroupNames());
    auto match = input->createMatch(std::move(captureGroups));
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(match != nullptr, "Expected match, got null"_el);
    return match;
}

auto RegEx::match(const Input16Ptr &input) const -> Match16Ptr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    const auto engine = this->engine();
    auto state = engine->createState(input);
    if (engine->match(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    auto captureGroups = state->createCaptureGroups(engine->captureGroupNames());
    auto match = input->createMatch(std::move(captureGroups));
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(match != nullptr, "Expected match, got null"_el);
    return match;
}

auto RegEx::match(const Input32Ptr &input) const -> Match32Ptr {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    const auto engine = this->engine();
    auto state = engine->createState(input);
    if (engine->match(*state) == impl::EngineHasMatch::No) {
        return {};
    }
    auto captureGroups = state->createCaptureGroups(engine->captureGroupNames());
    auto match = input->createMatch(std::move(captureGroups));
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(match != nullptr, "Expected match, got null"_el);
    return match;
}

}
