// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Engine.hpp"

#include "EngineDebug.hpp"
#include "EngineState.hpp"

#include <ranges>

namespace erbsland::re::impl {

auto Engine::match(EngineState &state) const -> EngineHasMatch {
    return run(state);
}

auto Engine::fullMatch(EngineState &state) const -> EngineHasMatch {
    state.setFlag(EngineFlag::FullMatch);
    return run(state);
}

auto Engine::findFirst(EngineState &state) const -> EngineHasMatch {
    state.setFlag(EngineFlag::FindFirst);
    return run(state);
}

auto Engine::run(EngineState &state) const -> EngineHasMatch {
    ERBSLAND_RE_ENGINE_DEBUG_REC_STATE(EDMType::Run, "Engine::run()"_el, state);
    // Entry point on the ＥＲＢＳＬＡＮＤ regular expression engine
    // At this point: current = EOF, next = first input character.
    // Next, process all initial flow operations until the next thread list is filled with character operations.
    prepareNext(state, state.createNewThread(state.next.position), 0);

    // Stop early if we have no more threads to process.
    if (!state.hasNext()) {
        // In `FindFirst` mode, scan forward if we haven't a match yet.
        if (state.isFlagSet(EngineFlag::FindFirst) && !state.isAtEnd() && !state.hasMatch()) {
            return mainLoop(state);
        }
        return state.hasMatch() ? EngineHasMatch::Yes : EngineHasMatch::No;
    }

    return mainLoop(state);
}

auto Engine::mainLoop(EngineState &state) const -> EngineHasMatch {
    // Loop until we:
    // - have an end request.
    // - reach the end of the input.
    // - have no more threads to process.
    while (!state.hasEndRequest() && !state.isAtEnd()) {
        // Keep the watchdog clock ticking.
        state.watchdog.tick(state.current.position);

        // Make the next list the current one.
        state.advanceToNext();

        // process all current threads.
        auto hasMatch = EngineHasMatch::No;
        for (std::size_t threadIndex = 0U; threadIndex < state.current.threads.size(); ++threadIndex) {
            auto &thread = state.current.threads[threadIndex];
            if (hasMatch == EngineHasMatch::Yes) {
                // close all following threads in the current list, they will never be able to beat this match.
                state.endThreadWithoutMatch(thread);
                continue;
            }
            // All threads must be on a character operation.
            auto operation = _programDecoder.peekOperation(thread.programCounter);
            EngineResult operationResult;
            switch (operation.raw()) {
            case Operation::Char:
                operationResult = handleChar(state, thread, false, false);
                break;
            case Operation::CiChar:
                operationResult = handleChar(state, thread, true, false);
                break;
            case Operation::NotChar:
                operationResult = handleChar(state, thread, false, true);
                break;
            case Operation::NotCiChar:
                operationResult = handleChar(state, thread, true, true);
                break;
            case Operation::Sequence:
                operationResult = handleSequence(state, thread, false);
                break;
            case Operation::CiSequence:
                operationResult = handleSequence(state, thread, true);
                break;
            case Operation::Category:
                operationResult = handleCategory(state, thread, false);
                break;
            case Operation::NotCategory:
                operationResult = handleCategory(state, thread, true);
                break;
            case Operation::Class:
                operationResult = handleClass(state, thread, false, false);
                break;
            case Operation::CiClass:
                operationResult = handleClass(state, thread, true, false);
                break;
            case Operation::NotClass:
                operationResult = handleClass(state, thread, false, true);
                break;
            case Operation::NotCiClass:
                operationResult = handleClass(state, thread, true, true);
                break;
            case Operation::Any:
                operationResult = handleAny(state, thread);
                break;
            default:
                throwError("Unexpected operation. Expected a character operation."_el);
            }
            if (operationResult == EngineResult::Continue) {
                if (prepareNext(state, thread, threadIndex) == EngineHasMatch::Yes) {
                    // if we have a MATCH or SUCCESS, stop all threads with lower priority.
                    hasMatch = EngineHasMatch::Yes;
                }
            } else {
                state.endThreadWithoutMatch(thread);
            }
        }

        // For *find-first* mode, keep starting low-priority threads while we have no match.
        if (state.isFlagSet(EngineFlag::FindFirst) && !state.hasMatch()) {
            // 1. The new thread is started at the next position.
            //    Simulating as there was an `ANY` operation executed in the current list.
            // 2. As thread-index, we use the number of threads in the current list to give this new thread
            //    the lowest possible priority.
            // 3. Ignore the result because this is the thread with the lowest priority.
            prepareNext(state, state.createNewThread(state.next.position), state.current.threads.size());
        } else if (!state.hasNext()) {
            break; // if there are no further threads to process, stop this loop.
        }
    }
    return state.hasMatch() ? EngineHasMatch::Yes : EngineHasMatch::No;
}

auto Engine::prepareNext(EngineState &state, const EngineThread &thread, const std::size_t threadIndex) const
    -> EngineHasMatch {

    state.work.initWorkList(thread);
    const auto programSize = _data->program.size();
    auto hasMatch = EngineHasMatch::No;
    while (state.work.hasWork()) {
        state.watchdog.tick(state.current.position);
        auto &work = state.work.currentWork();

        if (hasMatch == EngineHasMatch::Yes) {
            state.work.endWorkWithoutMatch(work);
            continue;
        }
        if (work.programCounter >= programSize) {
            // Coverage: should never happen for generated code.
            // This situation can happen with user-compiled programs that do not end in a MATCH operation.
            state.work.endWorkWithoutMatch(work);
            continue;
        }

        auto operation = _programDecoder.peekOperation(work.programCounter);
        if (operation != Operation::None && !operation.isFlow()) {
            state.addNextThread(work, operation.isCaseInsensitive());
            state.work.popWork();
            continue;
        }
        handleFlowOperation(state, work, threadIndex, hasMatch);
    }
    return hasMatch;
}

}
