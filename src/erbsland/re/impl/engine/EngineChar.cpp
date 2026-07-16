// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Engine.hpp"

#include "EngineDebug.hpp"

#include "../error/InternalError.hpp"

#include <format>

namespace erbsland::re::impl {

auto Engine::handleChar(
    EngineState &state, EngineThread &thread, const bool caseInsensitive, const bool isNegated) const -> EngineResult {

    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto character = _programDecoder.readChar(thread.programCounter);
    const auto testedCharacter = caseInsensitive ? state.current.caseFoldedCharacter : state.current.character;
    auto matches = (testedCharacter == character);
    if (isNegated) {
        matches = !matches;
    }
    ERBSLAND_RE_ENGINE_DEBUG_MOP1(character, matches);
    return matches ? EngineResult::Continue : EngineResult::Stop;
}

auto Engine::handleSequence(EngineState &state, EngineThread &thread, const bool caseInsensitive) const
    -> EngineResult {

    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto saveProgramCounter = thread.programCounter;
    auto [offset, length] = _programDecoder.readSequence(thread.programCounter);
    const auto atEnd = ((thread.sequenceCounter + 1) >= length);
#ifdef ERBSLAND_RE_ENGINE_DEBUG_ENABLED
    const auto debugOriginalOffset = offset;
    const auto debugSequenceCounter = thread.sequenceCounter;
#endif
    offset += thread.sequenceCounter;
    if (offset >= _data->sequenceData.size()) {
        throwError("Invalid character sequence offset"_el);
    }
    if (atEnd) {
        thread.sequenceCounter = 0;
    } else {
        thread.sequenceCounter += 1;
        thread.programCounter = saveProgramCounter; // repeat this operation.
    }
    const auto character = _data->sequenceData[offset];
    const auto testedCharacter = caseInsensitive ? state.current.caseFoldedCharacter : state.current.character;
    const auto matches = (testedCharacter == character);
    ERBSLAND_RE_ENGINE_DEBUG_MOP2(
        text::StringFormat{"${:04X} + {} ({})"}.build(debugOriginalOffset, debugSequenceCounter, character),
        length,
        matches);
    return matches ? EngineResult::Continue : EngineResult::Stop;
}

auto Engine::handleCategory(EngineState &state, EngineThread &thread, const bool isNegated) const -> EngineResult {

    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto category = _programDecoder.readCategory(thread.programCounter);
    auto matches = state.isCurrentInCategory(category);
    if (isNegated) {
        matches = !matches;
    }
    ERBSLAND_RE_ENGINE_DEBUG_MOP1(category.toShortString(), matches);
    return matches ? EngineResult::Continue : EngineResult::Stop;
}

auto Engine::handleClass(
    EngineState &state, EngineThread &thread, const bool caseInsensitive, const bool isNegated) const -> EngineResult {

    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto classIndex = _programDecoder.readClass(thread.programCounter);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(classIndex < charClassData().size(), "Class index out of bounds"_el);
    const auto &charClass = charClassData()[classIndex];
    const auto testedCharacter = caseInsensitive ? state.current.caseFoldedCharacter : state.current.character;
    auto matches = charClass.matches(testedCharacter);
    if (isNegated) {
        matches = !matches;
    }
    ERBSLAND_RE_ENGINE_DEBUG_MOP1(classIndex, matches);
    return matches ? EngineResult::Continue : EngineResult::Stop;
}

auto Engine::handleAny(EngineState &, EngineThread &thread) const -> EngineResult {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    _programDecoder.skipOperation(thread.programCounter);
    ERBSLAND_RE_ENGINE_DEBUG_MOP0(true);
    return EngineResult::Continue;
}

}
