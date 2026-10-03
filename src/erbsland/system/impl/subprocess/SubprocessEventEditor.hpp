// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SubprocessState_fwd.hpp"

#include "../../SubprocessEventEditor.hpp"

namespace erbsland::system::impl {

/// Typed handler editor retaining only the process workflow, not the source facade.
/// @tested{SubprocessEventTest}
class SubprocessEventEditor final : public system::SubprocessEventEditor {
public:
    /// Create the stable process editor.
    /// @param source Source owning the editor.
    /// @param target Owner event loop.
    /// @param state Synchronized handler storage.
    SubprocessEventEditor(
        event::EventSourcePtr source, event::EventsPtr target, std::shared_ptr<SubprocessState> state);

public: // implement SubprocessEventEditor
    auto onStarted(EventFn callback) -> system::SubprocessEventEditor & override;
    auto onOutputTruncated(EventFn callback) -> system::SubprocessEventEditor & override;
    auto onExit(ExitFn callback) -> system::SubprocessEventEditor & override;
    auto onError(ErrorFn callback) -> system::SubprocessEventEditor & override;
    auto onFinal(EventFn callback) -> system::SubprocessEventEditor & override;

    auto onStandardOutput(OutputFn callback) -> system::SubprocessEventEditor & override;
    auto onStandardError(OutputFn callback) -> system::SubprocessEventEditor & override;

private:
    /// Require editing to occur on the configured owner loop.
    void verifyOwner() const;

private:
    std::shared_ptr<SubprocessState> _state; ///< Handler storage shared with native workflow.
};

}
