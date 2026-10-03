// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessId.hpp"
#include "Subprocess_fwd.hpp"
#include "SubprocessEventEditor.hpp"
#include "SubprocessExitStatus.hpp"
#include "SubprocessOptions.hpp"

#include "impl/subprocess/SubprocessState_fwd.hpp"
#include "impl/SystemBackend_fwd.hpp"

#include "../event/EventSource.hpp"
#include "../path/Path.hpp"
#include "../text/StringList.hpp"
#include "../time/TimeDelta.hpp"

#include <optional>

namespace erbsland::system {

/// Shared owner of one directly launched child with optional owner-loop events.
/// Releasing the last owner terminates and reaps a running child. Event handlers must not strongly capture their
/// source.
/// @seedoc{/reference/system/system_services}
/// @tested{SubprocessInteropTest SubprocessEventTest}
class Subprocess final : public event::EventSource {
    friend class impl::SystemBackend;

public:
    ~Subprocess() override;

public: // factories
    /// Prepare a one-shot child without launching it or requiring an event loop.
    /// @param executable Direct executable path; no shell interpretation or PATH lookup.
    /// @param arguments Arguments following argv[0], valid UTF-8 without null bytes.
    /// @param options Validated environment, capture, and output settings.
    /// @return The inactive shared owner.
    [[nodiscard]] static auto create(
        const path::Path &executable, const text::StringList &arguments = {}, const SubprocessOptions &options = {})
        -> SubprocessPtr;

public: // lifecycle
    /// Launch this prepared child once.
    /// @throws PlatformError If native launch fails; event users also receive onError then onFinal.
    void start();
    /// Get the native identifier, invalid before launch and retained after exit.
    [[nodiscard]] auto processId() const noexcept -> ProcessId;
    /// Test whether the launched child is still running.
    [[nodiscard]] auto isRunning() -> bool;
    /// Get a synchronized exit-status snapshot.
    [[nodiscard]] auto exitStatus() const -> std::optional<SubprocessExitStatus>;
    /// Wait for workflow completion without pumping or waiting for event delivery.
    /// @return The native exit status.
    /// @throws PlatformError If a native launch, read, or wait operation failed.
    /// @throws LogicFailure If start() has not been called.
    [[nodiscard]] auto wait() -> SubprocessExitStatus;
    /// Wait until completion or a non-negative timeout.
    /// @param timeout Non-negative duration to wait for native exit and output drainage. Zero polls the
    /// synchronized workflow once. Expiry does not terminate the child or discard output.
    /// This wait neither pumps the owner loop nor waits for callback delivery.
    /// @return Exit status, or no value on timeout.
    /// @throws PlatformError If a native operation failed.
    /// @throws LogicFailure If start() has not been called.
    /// @throws ParameterFailure If timeout is negative.
    [[nodiscard]] auto wait(time::TimeDelta timeout) -> std::optional<SubprocessExitStatus>;
    /// Request native termination.
    /// @throws PlatformError If termination fails.
    void terminate();
    /// Force native termination.
    /// @throws PlatformError If termination fails.
    void kill();

public: // output
    /// Get the retained stdout prefix; event-routed bytes are not retained.
    /// One initial UTF-8 BOM is removed before capture unless disabled in SubprocessOptions.
    [[nodiscard]] auto standardOutput() const -> text::String;
    /// Get the retained stderr prefix, empty when stderr is merged.
    /// One initial UTF-8 BOM is removed before capture unless disabled in SubprocessOptions.
    [[nodiscard]] auto standardError() const -> text::String;
    /// Test whether stdout capture or event output overflowed.
    [[nodiscard]] auto wasStandardOutputTruncated() const noexcept -> bool;
    /// Test whether stderr capture or event output overflowed.
    [[nodiscard]] auto wasStandardErrorTruncated() const noexcept -> bool;

public: // implement EventSource
    /// Access the stable editor, lazily binding the current loop on first use.
    /// Must run on the owner loop. Past events are not replayed.
    /// @return The source-owned editor.
    [[nodiscard]] auto events() -> SubprocessEventEditor & override;

private:
    /// Prepare the internal workflow.
    explicit Subprocess(std::shared_ptr<impl::SubprocessState> state);
    /// Bind events and register backend polling.
    void bindEvents(const event::EventsPtr &owner);

private:
    std::shared_ptr<impl::SubprocessState> _state;  ///< Synchronized process workflow.
    std::unique_ptr<SubprocessEventEditor> _editor; ///< Lazily constructed handler editor.
};

}
