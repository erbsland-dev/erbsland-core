// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupManager.hpp"
#include "EngineCharacterState.hpp"
#include "EngineDebug.hpp"
#include "EngineFlags.hpp"
#include "EngineThread.hpp"
#include "EngineWork.hpp"
#include "Watchdog.hpp"

#include "../text/Category.hpp"

#include "../../../text/Literals.hpp"
#include "../../CharAndPosition.hpp"
#include "../../Input.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <utility>

namespace erbsland::re::impl {

using namespace text::literals;

class EngineState;
using EngineStatePtr = std::unique_ptr<EngineState>;

/// The state of the engine.
class EngineState final {
public:
    /// Create a new state for the given input.
    explicit EngineState(
        InputBasePtr input, const std::size_t captureGroupCount, const EngineFlags flags = EngineFlags{}) :
        _flags{flags},
        _input{std::move(input)},
        _captureGroupManager{CaptureGroupManager::create(captureGroupCount, flags.isSet(EngineFlag::AtomicGroups))} {}

    /// Default constructor for unit tests.
    /// @note Do not use this in production code, it is only for testing purposes.
    EngineState() : _input{nullptr}, _captureGroupManager{CaptureGroupManager::create(1, false)} {}

    // defaults
    EngineState(const EngineState &) = delete;
    EngineState(EngineState &&) = delete;
    auto operator=(const EngineState &) -> EngineState & = delete;
    auto operator=(EngineState &&) -> EngineState & = delete;
    ~EngineState() = default;

public: // Initialize
    /// Initialize this state.
    /// Called *once* when the state is created.
    void initialize() {
        // tools
        _captureGroupManager->initialize();

        // watchdog
        watchdog.reset();

        // character stream
        current.threads.reserve(32);
        current.position = 0;
        current.character = text::Char::endOfData();
        next.threads.reserve(32);
        const auto firstInput = _input->read();
        next.character = firstInput.character;
        next.position = firstInput.position;
        if (_flags.isSet(EngineFlag::FoldCRLF)) {
            foldCRLF();
        }

        // match
        _bestMatch = cNoCaptureGroup;
        _endRequested = false;

        // work
        work.initialize();
    }

    /// Reset the state to start the next *find first*.
    void resetForNextFind() {
        // tools
        _captureGroupManager->resetForNextFind();

        // watchdog
        watchdog.reset();

        // match
        _bestMatchThreadIndex = 0;
        _bestMatch = cNoCaptureGroup;
        _endRequested = false;

        // work
        work.resetForNextFind();
        current.resetForNextFind();
        next.resetForNextFind();
    }

public: // engine flags
    /// Set a flag.
    void setFlag(const EngineFlag flag) noexcept { _flags.set(flag); }

    [[nodiscard]] constexpr auto isFlagSet(const EngineFlag flag) const noexcept -> bool { return _flags.isSet(flag); }

public: // input handling
    /// Test if we currently are at the start of the input.
    /// This tests if the *next* character will be the *first* character.
    /// ```
    ///     C | N
    /// _   _ V a   b
    /// ```
    [[nodiscard]] auto isAtStart() const noexcept -> bool {
        // We are at the start as long as the current character is the artificial EOD sentinel and we
        // have not advanced past the initial input position. This also correctly matches empty input.
        return current.character.isEndOfData() && current.position == 0;
    }
    /// Test if we currently are at the end of the input.
    /// This tests if there is no next character.
    /// ```
    ///     C | N
    /// y   z V _   _
    /// ```
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return next.character.isEndOfData(); }

    /// Test the character category of the current character.
    [[nodiscard]] auto isCurrentInCategory(const Category category) noexcept -> bool {
        if (current.categoryMask == Category::cNotComputed) {
            current.categoryMask = Category::maskFor(current.character);
        }
        return (current.categoryMask & category.mask()) == category.mask();
    }

    /// Test the character category of the next character.
    [[nodiscard]] auto isNextInCategory(const Category category) noexcept -> bool {
        if (next.categoryMask == Category::cNotComputed) {
            next.categoryMask = Category::maskFor(next.character);
        }
        return (next.categoryMask & category.mask()) == category.mask();
    }

public: // threads
    /// Create a new thread that starts the program at position zero.
    /// @param startPosition The capture start position for this thread.
    /// @return The new thread.
    [[nodiscard]] auto createNewThread(const InputPosition startPosition) noexcept -> EngineThread {
        auto initialThread = EngineThread{0};
        initialThread.captureGroupSet = _captureGroupManager->createGroupSet(startPosition);
        return initialThread;
    }

    /// Test if there are threads to continue with the next character.
    [[nodiscard]] auto hasNext() const noexcept -> bool { return !next.threads.empty(); }

    /// Advance to the next state.
    /// - This will move all threads from the next-list into the current-list.
    /// - This will also move the next-character into the current-character.
    /// - A new next-character is read from the input stream.
    void advanceToNext() {
#ifdef ERBSLAND_RE_ENGINE_DEBUG_ENABLED
        const auto debugOldCurrentPosition = current.position;
        const auto debugOldNextPosition = next.position;
        const auto debugOldCharacter = current.character;
#endif
        current.swap(next);
        if (current.hasCaseInsensitiveOperations) {
            current.caseFoldedCharacter = current.character.caseFolded();
        }
        next.clear();
        const auto nextInput = _input->read();
        next.character = nextInput.character;
        next.position = nextInput.position;
        if (_flags.isSet(EngineFlag::FoldCRLF)) {
            foldCRLF();
        }
#ifdef ERBSLAND_RE_ENGINE_DEBUG_ENABLED
        ERBSLAND_RE_ENGINE_DEBUG(
            EDMType::Advance,
            text::StringFormat{
                "Advance Cursor: {:04X}/{:04X} (c:{}/n:{}) =>  {:04X}/{:04X} (c:{}/n:{})",
            }
                .build(
                    debugOldCurrentPosition,
                    debugOldNextPosition,
                    debugOldCharacter,
                    current.character,
                    current.position,
                    next.position,
                    current.character,
                    next.character));
#endif
    }

    /// Fold CR/LF into LF.
    /// If a `CR/LF` sequence is found, it will be folded into a 2-byte `LF` sequence.
    /// The pattern will only see `LF`, but when matched or copied, a `CR/LF` sequence will be preserved.
    /// If only a `CR` is found, that isn't followed by an `LF`, it will be preserved as a single `CR` character.
    void foldCRLF() {
        if (next.character == U'\r') {
            auto const [character, position] = _input->peek();
            if (character == U'\n') {
                _input->skip(unit::CpLength::one());
                next.character = U'\n'; // overwrite
            }
        }
    }

    /// Add a given thread to the next-list, but only if it is a unique path.
    /// @param thread The thread to add.
    /// @param isCaseInsensitiveOperation If this threat is at a case-insensitive operation.
    void addNextThread(EngineThread thread, const bool isCaseInsensitiveOperation) {
        // fold equal threads as they will result in equal outcomes.
        for (auto &existingThread : next.threads) {
            if (existingThread == thread) { // merge is possible?
                _captureGroupManager->release(thread.captureGroupSet);
                return;
            }
        }
        if (next.threads.size() >= limits::maximumActiveThreads) {
            throw RegExError{
                ErrorCategory::Limit,
                "Failed to match regular expression"_el,
                "The engine exceeded the maximum active thread count."_el};
        }
        if (isCaseInsensitiveOperation) {
            next.hasCaseInsensitiveOperations = true;
        }
        next.threads.emplace_back(thread);
    }

    /// Clone a thread.
    /// @param thread The thread to clone.
    /// @return The cloned thread.
    [[nodiscard]] auto cloneThread(const EngineThread &thread) noexcept -> EngineThread {
        _captureGroupManager->allocate(thread.captureGroupSet);
        return thread;
    }

    /// End a thread.
    /// @param thread The thread to end.
    void endThreadWithoutMatch(const EngineThread &thread) { _captureGroupManager->release(thread.captureGroupSet); }

    /// Update the state when a thread was successful.
    /// @param thread The successful thread.
    /// @param threadIndex The thread index of the successful thread (for priority check).
    void endThreadWithMatch(EngineThread &thread, const std::size_t threadIndex) {
        if (!hasMatch() || threadIndex <= _bestMatchThreadIndex) {
            _bestMatchThreadIndex = threadIndex;
            // we write the new end position into the group, only if we have a new best match
            thread.captureGroupSet = _captureGroupManager->stopCapture(thread.captureGroupSet, 0, next.position);
            _captureGroupManager->release(_bestMatch);  // release the previously best match (if it was set).
            _bestMatch = thread.captureGroupSet;
            _captureGroupManager->allocate(_bestMatch); // allocate the new best match.
        }
        _captureGroupManager->release(thread.captureGroupSet);
    }

    /// Test if end of processing is requested.
    [[nodiscard]] auto hasEndRequest() const noexcept -> bool { return _endRequested; }

    /// Request an end of processing after the current character.
    void requestEnd() { _endRequested = true; }

    /// Prune the list of current threads.
    /// Called on STOP ATOMIC to prune the threads.
    /// @param referencesToPrune A list of capture group references to prune.
    /// @param threadIndex The current thread index, so start pruning *after* this index.
    void pruneCurrentList(const CaptureGroupSetReferenceList &referencesToPrune, const std::size_t threadIndex) {

        const auto firstPrunedThreadIndex = threadIndex + 1;

        // Only threads *after* `currentThreadIndex` have to be tested for pruning.
        // Test if there are no threads after the current one.
        if (referencesToPrune.empty() || firstPrunedThreadIndex >= current.threads.size()) {
            return;
        }

        // Prepare the range for pruning tests.
        const auto begin =
            current.threads.begin() + static_cast<EngineThreadList::difference_type>(firstPrunedThreadIndex);
        const auto end = current.threads.end();

        // Erase all threads with lower-priority that are affected by the references.
        current.threads.erase(
            std::remove_if(
                begin,
                end,
                [&](const auto &thread) -> bool {
                    const auto doPrune = std::ranges::any_of(
                        referencesToPrune, [&](const auto &ref) -> bool { return thread.captureGroupSet == ref; });
                    if (doPrune) {
                        endThreadWithoutMatch(thread);
                    }
                    return doPrune;
                }),
            end);
    }

public: // capture groups and result.
    /// Test if we had a successful match
    [[nodiscard]] auto hasMatch() const noexcept -> bool { return _bestMatch != cNoCaptureGroup; }

    /// Access the capture manager.
    [[nodiscard]] auto captureGroupManager() noexcept -> CaptureGroupManager & { return *_captureGroupManager; }
    /// Create capture groups from the current state after a successful match.
    /// @param names The capture group names.
    [[nodiscard]] auto createCaptureGroups(const CaptureGroupNames &names) const -> CaptureGroupList {
        return _captureGroupManager->createCaptureGroupList(_bestMatch, names);
    }

#ifdef ERBSLAND_RE_ENGINE_DEBUG_ENABLED
    [[nodiscard]] auto toDebugString() -> text::String {
        return text::StringFormat{"State(\n"
                                  "  Current: {}\n"
                                  "  Next: {}\n"
                                  "  Best match: {}\n"
                                  "  End requested: {}\n"
                                  "  Watchdog(tick={}, positionAtLastTimeoutCheck={})\n"
                                  ")\n"}
            .build(
                current.toDebugString(),
                next.toDebugString(),
                _bestMatch,
                _endRequested,
                watchdog.tickCount(),
                watchdog.positionAtLastTimeoutCheck());
    }
#endif

private:
    // main
    EngineFlags _flags;  ///< Flags for the engine.
    InputBasePtr _input; ///< The input the engine is operating on.

    // tools
    CaptureGroupManagerPtr _captureGroupManager; ///< The capture group manager.

    // match
    std::size_t _bestMatchThreadIndex{0};   ///< The thread index of the best match.
    CaptureGroupSetReference _bestMatch{0}; ///< The best match so far.
    bool _endRequested = false;             ///< Flag if no further paths shall be explored.

public:                                     // parts of this state.
    Watchdog watchdog{};
    EngineWork work{_captureGroupManager};
    EngineCharacterState current; ///< The currently processed threads and character.
    EngineCharacterState next;    ///< The next threads and character.
};

}
