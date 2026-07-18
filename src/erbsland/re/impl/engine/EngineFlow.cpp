// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Engine.hpp"

#include "EngineDebug.hpp"
#include "EngineState.hpp"

#include <cstdint>
#include <limits>
#include <type_traits>

namespace erbsland::re::impl {

void Engine::requireProgramCounterInBounds(const std::size_t programCounter) const {
    if (programCounter >= _data->program.size()) {
        throwError("Program counter out of bounds"_el); // should never happen.
    }
}

void Engine::requireCounterIndexInBounds(const CounterIndex counterIndex) {
    if (counterIndex >= limits::maximumCounterCount) {
        throwError("Counter index out of bounds"_el); // should never happen.
    }
}

void Engine::handleFlowOperation(
    EngineState &state, EngineThread &thread, const std::size_t threadIndex, EngineHasMatch &hasMatch) const {

    const auto operation = _programDecoder.peekOperation(thread.programCounter);
    switch (operation.raw()) {
    case Operation::None:
        _programDecoder.skipOperation(thread.programCounter);
        break;
    case Operation::Split:
        handleSplitOperation(state, thread);
        break;
    case Operation::Jump:
        handleJumpOperation(thread);
        break;
    case Operation::Match:
        handleMatchOperation(state, thread, threadIndex, hasMatch);
        break;
    case Operation::NotMatch:
        state.work.endWorkWithoutMatch(thread);
        break;
    case Operation::Success:
        handleSuccessOperation(state, thread, threadIndex, hasMatch);
        break;
    case Operation::Failure:
        state.work.endWorkWithoutMatch(thread);
        state.requestEnd();
        break;
    case Operation::Anchor:
        handleAnchorOperation(state, thread);
        break;
    case Operation::StartCapture:
        handleCaptureOperation(state, thread, true);
        break;
    case Operation::StopCapture:
        handleCaptureOperation(state, thread, false);
        break;
    case Operation::StartAtomic:
        handleStartAtomic(state, thread);
        break;
    case Operation::StopAtomic:
        handleStopAtomic(state, thread, threadIndex);
        break;
    case Operation::Counter:
        handleCounterOperation(thread);
        break;
    case Operation::AddCounter:
        handleAddCounterOperation(thread);
        break;
    case Operation::Maximum:
        handleMaximumOperation(state, thread);
        break;
    case Operation::SkipIfMaximum:
        handleSkipIfMaximumOperation(state, thread);
        break;
    case Operation::Minimum:
        handleMinimumOperation(state, thread);
        break;
    case Operation::AssertCategory:
        handleAssertCategoryOperation(state, thread, false);
        break;
    case Operation::NotAssertCategory:
        handleAssertCategoryOperation(state, thread, true);
        break;
    default:
        throwError("Unexpected flow operation"_el);
    }
}

void Engine::handleAssertCategoryOperation(EngineState &state, EngineThread &thread, const bool isNegated) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto category = _programDecoder.readCategory(thread.programCounter);
    auto matches = state.isCurrentInCategory(category);
    if (isNegated) {
        matches = !matches;
    }
    ERBSLAND_RE_ENGINE_DEBUG_MOP0(matches);
    if (!matches) {
        state.work.endWorkWithoutMatch(thread);
    }
}

void Engine::handleSplitOperation(EngineState &state, EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto [programCounterA, programCounterB] = _programDecoder.readSplit(thread.programCounter);
    ERBSLAND_RE_ENGINE_DEBUG_OP2PC(programCounterA, programCounterB);
    thread.programCounter = programCounterA;
    requireProgramCounterInBounds(thread.programCounter);

    auto newWork = state.cloneThread(thread);
    newWork.programCounter = programCounterB;
    requireProgramCounterInBounds(newWork.programCounter);
    state.work.addWork(newWork);
}

void Engine::handleJumpOperation(EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto newProgramCounter = _programDecoder.readJump(thread.programCounter);
    ERBSLAND_RE_ENGINE_DEBUG_OP1PC(newProgramCounter);
    thread.programCounter = newProgramCounter;
    requireProgramCounterInBounds(thread.programCounter);
}

void Engine::handleMatchOperation(
    EngineState &state, EngineThread &thread, const std::size_t threadIndex, EngineHasMatch &hasMatch) const {

    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    ERBSLAND_RE_ENGINE_DEBUG_MOP0(!(state.isFlagSet(EngineFlag::FullMatch) && !state.isAtEnd()));
    if (state.isFlagSet(EngineFlag::FullMatch) && !state.isAtEnd()) {
        state.work.endWorkWithoutMatch(thread);
        return;
    }
    ERBSLAND_RE_ENGINE_DEBUG_REC_STATE(EDMType::Run, "Engine::handleMatchOperation()"_el, state);
    hasMatch = EngineHasMatch::Yes;
    state.endThreadWithMatch(thread, threadIndex);
    state.work.popWork();
}

void Engine::handleSuccessOperation(
    EngineState &state, EngineThread &thread, const std::size_t threadIndex, EngineHasMatch &hasMatch) const {

    handleMatchOperation(state, thread, threadIndex, hasMatch);
    if (hasMatch == EngineHasMatch::Yes) {
        state.requestEnd();
    }
}

void Engine::handleAnchorOperation(EngineState &state, EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto anchor = _programDecoder.readAnchor(thread.programCounter);
    const auto isMatch = testAnchor(state, anchor);
    ERBSLAND_RE_ENGINE_DEBUG_MOP1(anchor.toString(), isMatch);
    if (!isMatch) {
        state.work.endWorkWithoutMatch(thread);
    }
}

void Engine::handleCaptureOperation(EngineState &state, EngineThread &thread, const bool isStart) const {
    // The capture group index in the VM is 0-based for actual capture groups.
    // The CaptureGroupManager uses 0 for the full match, and 1+ for capture groups.
    // Therefore, we add 1 to the index to get the correct group in the manager.
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto captureGroup = _programDecoder.readCapture(thread.programCounter);
    ERBSLAND_RE_ENGINE_DEBUG_OP1((captureGroup + 1));
    if (isStart) {
        thread.captureGroupSet =
            state.captureGroupManager().startCapture(thread.captureGroupSet, captureGroup + 1, state.next.position);
    } else {
        thread.captureGroupSet =
            state.captureGroupManager().stopCapture(thread.captureGroupSet, captureGroup + 1, state.next.position);
    }
}

void Engine::handleCounterOperation(EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto [counterIndex, newValue] = _programDecoder.readCounter(thread.programCounter);
    requireCounterIndexInBounds(counterIndex);
    ERBSLAND_RE_ENGINE_DEBUG_OP2(counterIndex, newValue);
    thread.counter[counterIndex] = newValue;
}

void Engine::handleAddCounterOperation(EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto [counterIndex, value] = _programDecoder.readCounter(thread.programCounter);
    requireCounterIndexInBounds(counterIndex);
    ERBSLAND_RE_ENGINE_DEBUG_OP2(counterIndex, value);
    using LargerThanCounterType = std::conditional_t<sizeof(CounterIndex) >= 4, uint64_t, uint32_t>;
    static_assert(sizeof(LargerThanCounterType) > sizeof(CounterType));
    const auto current = static_cast<LargerThanCounterType>(thread.counter[counterIndex]);
    const auto newValue = current + static_cast<CounterType>(value);
    constexpr auto maxValue = static_cast<LargerThanCounterType>(std::numeric_limits<CounterType>::max());
    if (newValue > maxValue) {
        thread.counter[counterIndex] = maxValue;
    } else {
        thread.counter[counterIndex] = static_cast<CounterType>(newValue);
    }
}

void Engine::handleMaximumOperation(EngineState &state, EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto [counterIndex, maximumValue] = _programDecoder.readCounter(thread.programCounter);
    requireCounterIndexInBounds(counterIndex);
    ERBSLAND_RE_ENGINE_DEBUG_MOP2(counterIndex, maximumValue, (thread.counter[counterIndex] < maximumValue));
    if (thread.counter[counterIndex] >= maximumValue) {
        state.work.endWorkWithoutMatch(thread);
        return;
    }
    thread.counter[counterIndex] += 1;
}

void Engine::handleSkipIfMaximumOperation(EngineState &, EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto [counterIndex, maximumValue] = _programDecoder.readCounter(thread.programCounter);
    requireCounterIndexInBounds(counterIndex);
    ERBSLAND_RE_ENGINE_DEBUG_MOP2(counterIndex, maximumValue, (thread.counter[counterIndex] < maximumValue));
    if (thread.counter[counterIndex] < maximumValue) {
        thread.counter[counterIndex] += 1;
        return;
    }
    thread.counter[counterIndex] = 0;
    _programDecoder.skipOperation(thread.programCounter);
    requireProgramCounterInBounds(thread.programCounter);
}

void Engine::handleMinimumOperation(EngineState &state, EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto [counterIndex, minimumValue] = _programDecoder.readCounter(thread.programCounter);
    requireCounterIndexInBounds(counterIndex);
    ERBSLAND_RE_ENGINE_DEBUG_MOP2(counterIndex, minimumValue, (thread.counter[counterIndex] >= minimumValue));
    if (thread.counter[counterIndex] < minimumValue) {
        state.work.endWorkWithoutMatch(thread);
        return;
    }
    thread.counter[counterIndex] = 0;
}

auto Engine::testAnchor(EngineState &state, const TextAnchor anchor) const -> bool {
    // Anchors are a bit tricky to understand, as they are zero-width flow operations.
    // This engine processes all flow commands immediately, *before* the next character is
    // becoming the *current* character. Therefore, anchors looking one character into the future.
    constexpr auto unicodeWord = Category{Category::WordUnicode};
    constexpr auto asciiWord = Category{Category::WordAscii};
    switch (anchor.raw()) {
    case TextAnchor::None:
        return false;
    case TextAnchor::Start:
        return state.isAtStart();
    case TextAnchor::End:
        return state.isAtEnd();
    case TextAnchor::LineStart:
        return state.current.character == U'\n' || state.isAtStart();
    case TextAnchor::LineEnd:
        return state.next.character == U'\n' || state.isAtEnd();
    case TextAnchor::UnicodeWordBoundary:
        return state.isCurrentInCategory(unicodeWord) != state.isNextInCategory(unicodeWord);
    case TextAnchor::AsciiWordBoundary:
        return state.isCurrentInCategory(asciiWord) != state.isNextInCategory(asciiWord);
    case TextAnchor::NonUnicodeWordBoundary:
        return state.isCurrentInCategory(unicodeWord) == state.isNextInCategory(unicodeWord);
    case TextAnchor::NonAsciiWordBoundary:
        return state.isCurrentInCategory(asciiWord) == state.isNextInCategory(asciiWord);
    default:
        throwInternalError("Unknown anchor"_el);
    }
}

void Engine::handleStartAtomic(EngineState &state, EngineThread &thread) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto atomicGroupId = _programDecoder.readAtomic(thread.programCounter);
    ERBSLAND_RE_ENGINE_DEBUG_OP1(atomicGroupId);
    thread.captureGroupSet =
        state.captureGroupManager().markThreadWithAtomicGroupId(thread.captureGroupSet, atomicGroupId);
}

void Engine::handleStopAtomic(EngineState &state, EngineThread &thread, const size_t threadIndex) const {
    ERBSLAND_RE_ENGINE_DEBUG_OP_VAR();
    const auto atomicGroupId = _programDecoder.readAtomic(thread.programCounter);
    ERBSLAND_RE_ENGINE_DEBUG_OP1(atomicGroupId);

    const auto affectedReferences =
        state.captureGroupManager().getAllReferencesForAtomicGroup(thread.captureGroupSet, atomicGroupId);
    // Remove all affected threads with lower priority.
    // - Threads with higher priority are always processed first:
    //   - Threads in the "current-list" are ordered by priority, therefore, only threads after `threadIndex`
    //     have to be tested for pruning.
    //   - The work list is FIFO, so threads with lower priority are still in the work list.
    //     Yet *this* work thread (that is currently processed) must be excluded from pruning.
    //     Therefore, all entries starting from `workHead + 1` need to be tested for pruning.
    //   - *All* threads in the "next-list" were processed *before* this thread and, therefore, have higher priority,
    //     and we can ignore all of them.
    state.work.prune(affectedReferences);
    state.pruneCurrentList(affectedReferences, threadIndex);
}

void Engine::throwError(const text::String &message) {
    throw RegExError{ErrorCategory::Engine, "Failed to match regular expression"_el, message};
}

}
