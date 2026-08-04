// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Engine_fwd.hpp"
#include "EngineData.hpp"
#include "EngineMatch.hpp"
#include "EngineResult.hpp"
#include "EngineState.hpp"
#include "EngineThread.hpp"
#include "Operation.hpp"
#include "ProgramReader.hpp"

#include "../error/InternalError.hpp"
#include "../input/NullRejectingInput.hpp"
#include "../text/TextAnchor.hpp"

#include "../../Input.hpp"
#include "../../Settings.hpp"

#include <optional>

namespace erbsland::re::impl {

/// The engine for executing regular expressions.
/// Made for Thompson's construction algorithm.
class Engine : public std::enable_shared_from_this<Engine> {
private:
    struct PrivateTag {};

public:
    /// Create a new engine with the given data.
    /// @param data The engine data.
    /// @param settings The settings.
    explicit Engine(ConstEngineDataPtr data, Settings settings, PrivateTag) :
        _sharedData{std::move(data)},
        _data{_sharedData.get()},
        _settings{std::move(settings)},
        _programDecoder{_data->program} {}

    /// Create a new engine that stores its immutable data in the same allocation.
    explicit Engine(EngineData data, Settings settings, PrivateTag) :
        _ownedData{std::move(data)},
        _data{&*_ownedData},
        _settings{std::move(settings)},
        _programDecoder{_data->program} {}

    /// Create a new engine with the given data.
    /// @param data The engine data.
    /// @param settings The settings.
    /// @return A new instance of an engine.
    [[nodiscard]] static auto create(ConstEngineDataPtr data, Settings settings = {}) -> EnginePtr {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(data != nullptr, "Data must not be null"_el);
        return std::make_shared<Engine>(std::move(data), std::move(settings), PrivateTag{});
    }

    /// Create a new engine with inline immutable data.
    /// @param data The completed engine data.
    /// @param settings The settings.
    /// @return A new engine instance.
    [[nodiscard]] static auto create(EngineData data, Settings settings = {}) -> EnginePtr {
        return std::make_shared<Engine>(std::move(data), std::move(settings), PrivateTag{});
    }

    // defaults
    ~Engine() = default;

public: // configure
    /// Set an initial flag for the engine state.
    void setInitialFlag(const EngineFlag flag) noexcept { _initialFlags.set(flag); }

public: // accessors
    /// Access the engine data.
    [[nodiscard]] auto data() const noexcept -> ConstEngineDataPtr {
        if (_sharedData) {
            return _sharedData;
        }
        return ConstEngineDataPtr{shared_from_this(), _data};
    }
    /// Access the sequence data.
    [[nodiscard]] auto sequenceData() const noexcept -> const SequenceData & { return _data->sequenceData; }
    /// Access the character classes data.
    [[nodiscard]] auto charClassData() const noexcept -> const CharClassData & { return _data->charClassData; }
    /// Access the capture group names.
    [[nodiscard]] auto captureGroupNames() const noexcept -> const CaptureGroupNames & {
        return _data->captureGroupNames;
    }
    /// Access the program data.
    [[nodiscard]] auto program() const noexcept -> const Program & { return _data->program; }

public: // run the engine.
    /// Create a new state.
    /// We create the state as unique ptr on the heap to keep the frame in `RegEx` as small as possible.
    /// @param input The input to match against.
    /// @return A unique pointer to the created state.
    [[nodiscard]] auto createState(InputBasePtr input) const -> EngineStatePtr {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(input != nullptr, "Input must not be null"_el);
        if (!_settings.hasFeature(Feature::AcceptNullInInput)) {
            input = NullRejectingInput::create(std::move(input));
        }
        auto flags = _initialFlags;
        if (_data->hasAtomicGroups) {
            flags.set(EngineFlag::AtomicGroups);
        }
        auto state =
            std::make_unique<EngineState>(std::move(input), captureGroupNames().size() + 1, flags, _data->counterCount);
        state->initialize();
        if (_settings.timeout() != std::chrono::milliseconds::zero()) {
            state->watchdog.setTimeout(_settings.timeout());
        }
        return state;
    }

    /// Match input against the compiled program.
    /// @param state The state prepared for this operation.
    /// @return if a match was found, false otherwise.
    [[nodiscard]] auto match(EngineState &state) const -> EngineHasMatch;

    /// Match input against the compiled program.
    /// This method sets the `FullMatch` flag that requires any `MATCH` to be at the end of the input.
    /// @param state The state prepared for this operation.
    /// @return if a match was found, false otherwise.
    [[nodiscard]] auto fullMatch(EngineState &state) const -> EngineHasMatch;

    /// Find the first match in the input.
    /// @param state The state prepared for this operation.
    /// @return if a match was found, false otherwise.
    [[nodiscard]] auto findFirst(EngineState &state) const -> EngineHasMatch;

private:
    /// Run the engine with all
    /// @return if a match was found, false otherwise.
    [[nodiscard]] auto run(EngineState &state) const -> EngineHasMatch;

    /// The main loop of the engine.
    [[nodiscard]] auto mainLoop(EngineState &state) const -> EngineHasMatch;

    /// Process a thread.
    /// @param state The current state.
    /// @param thread The thread to continue.
    /// @param threadIndex The index of the thread in the current list for priority checks.
    auto prepareNext(EngineState &state, const EngineThread &thread, std::size_t threadIndex) const -> EngineHasMatch;

    /// Test if the program counter is within valid bounds.
    /// @param programCounter The program counter.
    void requireProgramCounterInBounds(std::size_t programCounter) const;

    /// Test if the counter-index is within valid bounds.
    static void requireCounterIndexInBounds(CounterIndex counterIndex);

    /// Handle flow operations.
    void handleFlowOperation(
        EngineState &state, EngineThread &thread, std::size_t threadIndex, EngineHasMatch &hasMatch) const;

    /// Handle the `SPLIT` operation.
    void handleSplitOperation(EngineState &state, EngineThread &thread) const;

    /// Handle the `JUMP` operation.
    void handleJumpOperation(EngineThread &thread) const;

    /// Handle the `MATCH` operation.
    void handleMatchOperation(
        EngineState &state, EngineThread &thread, std::size_t threadIndex, EngineHasMatch &hasMatch) const;

    /// Handle the `SUCCESS` operation.
    void handleSuccessOperation(
        EngineState &state, EngineThread &thread, std::size_t threadIndex, EngineHasMatch &hasMatch) const;

    /// Handle the `ANCHOR` operation.
    void handleAnchorOperation(EngineState &state, EngineThread &thread) const;

    /// Handle the `CAPTURE` operation.
    void handleCaptureOperation(EngineState &state, EngineThread &thread, bool isStart) const;

    /// Handle the `COUNTER` operation.
    void handleCounterOperation(EngineThread &thread) const;

    /// Handle the `ADD COUNTER` operation.
    void handleAddCounterOperation(EngineThread &thread) const;

    /// Handle the `MAXIMUM` operation.
    void handleMaximumOperation(EngineState &state, EngineThread &thread) const;

    /// Handle the `SKIP MAXIMUM` operation.
    void handleSkipIfMaximumOperation(EngineState &state, EngineThread &thread) const;

    /// Handle the `MINIMUM` operation.
    void handleMinimumOperation(EngineState &state, EngineThread &thread) const;

    /// Handle the `ASSERT CATEGORY` operation.
    void handleAssertCategoryOperation(EngineState &state, EngineThread &thread, bool isNegated) const;

    /// Test an anchor
    /// @param state The engine state.
    /// @param anchor The anchor to test.
    /// @return True if the anchor matches, false otherwise.
    [[nodiscard]] auto testAnchor(EngineState &state, TextAnchor anchor) const -> bool;

    /// Handle the `CHAR` operation.
    auto handleChar(EngineState &state, EngineThread &thread, bool caseInsensitive, bool isNegated) const
        -> EngineResult;

    /// Handle the `SEQUENCE` operation.
    auto handleSequence(EngineState &state, EngineThread &thread, bool caseInsensitive) const -> EngineResult;

    /// Handle the `CATEGORY` operation.
    auto handleCategory(EngineState &state, EngineThread &thread, bool isNegated) const -> EngineResult;

    /// Handle the `CLASS` operation.
    auto handleClass(EngineState &state, EngineThread &thread, bool caseInsensitive, bool isNegated) const
        -> EngineResult;

    /// Handle the `ANY` operation.
    auto handleAny(EngineState &state, EngineThread &thread) const -> EngineResult;

    /// Handle the `START ATOMIC` operation.
    void handleStartAtomic(EngineState &state, EngineThread &thread) const;

    /// Handle the `STOP ATOMIC` operation.
    void handleStopAtomic(EngineState &state, EngineThread &thread, size_t threadIndex) const;

    /// Throw an engine error.
    [[noreturn]] static void throwError(text::String message);

public:
    ConstEngineDataPtr _sharedData;       ///< Externally owned engine data, if supplied by the assembler/compiler.
    std::optional<EngineData> _ownedData; ///< Inline data used by allocation-sensitive compiler fast paths.
    const EngineData *_data;              ///< Active immutable engine data.
    EngineFlags _initialFlags;            ///< The initial flags for the engine state.
    Settings _settings;                   ///< The settings used when compiling the engine.
    ProgramReader _programDecoder;        ///< The program decoder.
};

}
