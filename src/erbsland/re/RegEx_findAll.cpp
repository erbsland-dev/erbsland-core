// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "Match.hpp"
#include "Match16.hpp"
#include "Match32.hpp"

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

auto RegEx::findAll(const text::String &text) const -> MatchGenerator {
    return findAll(impl::StringInput::create(text));
}

auto RegEx::findAll(const text::U16String &text) const -> Match16Generator {
    return findAll(impl::U16StringInput::create(text));
}

auto RegEx::findAll(const text::U32String &text) const -> Match32Generator {
    return findAll(impl::U32StringInput::create(text));
}

auto RegEx::findAll(const stream::TextInputStreamPtr input) const -> MatchGenerator {
    return findAll(impl::StreamInput::create(input));
}

auto RegEx::findAll(const InputPtr input) const -> MatchGenerator {
    // The input is intentionally copied into the coroutine frame.
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    auto lastPosition = state->current.position;
    while (_engine->findFirst(*state) == impl::EngineHasMatch::Yes) {
        auto captureGroups = state->createCaptureGroups(_engine->captureGroupNames());
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(!captureGroups.empty(), "Capture groups cannot be empty"_el);
        const auto captureRange = captureGroups.front().range();
        auto match = input->createMatch(std::move(captureGroups));
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(match != nullptr, "Expected match, got null"_el);
        co_yield std::move(match);
        if (state->isAtEnd()) {
            break;
        }
        if (captureRange.isEmpty() && lastPosition == state->current.position) {
            state->advanceToNext();
        }
        state->resetForNextFind();
        lastPosition = state->current.position;
    }
}

auto RegEx::findAll(const Input16Ptr input) const -> Match16Generator {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    auto lastPosition = state->current.position;
    while (_engine->findFirst(*state) == impl::EngineHasMatch::Yes) {
        auto captureGroups = state->createCaptureGroups(_engine->captureGroupNames());
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(!captureGroups.empty(), "Capture groups cannot be empty"_el);
        const auto captureRange = captureGroups.front().range();
        auto match = input->createMatch(std::move(captureGroups));
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(match != nullptr, "Expected match, got null"_el);
        co_yield std::move(match);
        if (state->isAtEnd()) {
            break;
        }
        if (captureRange.isEmpty() && lastPosition == state->current.position) {
            state->advanceToNext();
        }
        state->resetForNextFind();
        lastPosition = state->current.position;
    }
}

auto RegEx::findAll(const Input32Ptr input) const -> Match32Generator {
    if (input == nullptr) {
        throw err::ParameterError{"Input cannot be null."_el, "input"_el};
    }
    auto state = _engine->createState(input);
    auto lastPosition = state->current.position;
    while (_engine->findFirst(*state) == impl::EngineHasMatch::Yes) {
        auto captureGroups = state->createCaptureGroups(_engine->captureGroupNames());
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(!captureGroups.empty(), "Capture groups cannot be empty"_el);
        const auto captureRange = captureGroups.front().range();
        auto match = input->createMatch(std::move(captureGroups));
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(match != nullptr, "Expected match, got null"_el);
        co_yield std::move(match);
        if (state->isAtEnd()) {
            break;
        }
        if (captureRange.isEmpty() && lastPosition == state->current.position) {
            state->advanceToNext();
        }
        state->resetForNextFind();
        lastPosition = state->current.position;
    }
}

}
