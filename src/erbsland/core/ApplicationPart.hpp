// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPart_fwd.hpp"
#include "ApplicationPartCommandLine.hpp"
#include "ApplicationPartIdentifier_fwd.hpp"
#include "ApplicationPartManager_fwd.hpp"
#include "ApplicationPartState.hpp"

#include "impl/ApplicationPartManager_fwd.hpp"

#include "../event/EventTarget_fwd.hpp"
#include "../time/TimeDelta.hpp"
#include "../util/List.hpp"

#include <atomic>
#include <functional>
#include <mutex>

namespace erbsland::core {

/// Base class for an independently managed application component.
/// The `onInitialize()`, `onRunning()`, and `onStopping()` hooks run in the part's event loop.
/// Cleanup runs on the same thread after the loop exits; policy queries run on the manager's control target.
/// Command-line hooks run synchronously on their caller's thread.
/// @seedoc{/reference/core/application_framework}
/// @tested{ApplicationPartManagerTest ApplicationPartApplicationTest}
class ApplicationPart : public ApplicationPartCommandLine {
    friend class impl::ApplicationPartManager;

public:
    // defaults/deletions
    ApplicationPart() = default;
    ~ApplicationPart() override = default;
    ApplicationPart(const ApplicationPart &) = delete;
    auto operator=(const ApplicationPart &) -> ApplicationPart & = delete;
    ApplicationPart(ApplicationPart &&) = delete;
    auto operator=(ApplicationPart &&) -> ApplicationPart & = delete;

public:
    /// Return an empty dependency list.
    [[nodiscard]] static auto dependencies() -> ApplicationPartIdentifierList { return {}; }

public: // state and services
    /// Get the current lifecycle state.
    [[nodiscard]] auto state() const noexcept -> ApplicationPartState { return _state.load(); }
    /// Access the manager-local part registry.
    /// The part must have been prepared by a manager.
    [[nodiscard]] auto partManager() const -> ApplicationPartManagerAccess &;
    /// Access this part's event target.
    /// The part must have started.
    [[nodiscard]] auto loop() const -> event::EventTargetPtr;

public: // implement ApplicationPartCommandLine
    void registerCommandLineOptions(const options::OptionsPtr &options) override;
    void handleCommandLineOptions(const options::OptionValuesPtr &values) override;

protected: // lifecycle customization
    /// Decide whether this part starts during initial automatic startup.
    [[nodiscard]] virtual auto automaticStart() -> bool;
    /// Get the maximum graceful-shutdown time before the event loop is forced to quit.
    /// Expiry is reported to the manager as an ApplicationError.
    [[nodiscard]] virtual auto shutdownTimeout() const noexcept -> time::TimeDelta;
    /// Initialize this part in its dedicated event loop while its state is `Starting`.
    virtual void onInitialize();
    /// Begin work in a separate event callback after initialization returns and the part enters `Running`.
    /// The manager may start dependent parts before this callback completes.
    virtual void onRunning();
    /// Begin shutdown in this part's dedicated event loop.
    /// The default implementation calls `completeStopping()` immediately.
    virtual void onStopping();
    /// Clean up after the event loop has exited, on the dedicated part thread.
    virtual void cleanup() noexcept;
    /// Complete asynchronous stopping and request the part event loop to quit.
    void completeStopping() noexcept;

private: // manager access
    /// Bind this part to its prepared manager and optional runtime event target.
    void bind(ApplicationPartManagerAccessWeakPtr manager, event::EventTargetPtr events);
    /// Publish a manager-owned lifecycle state.
    void setState(ApplicationPartState state) noexcept;

private:
    std::atomic<ApplicationPartState> _state{ApplicationPartState::Uninitialized}; ///< Current lifecycle state.
    ApplicationPartManagerAccessWeakPtr _manager;                                  ///< Owning manager access.
    event::EventTargetPtr _events;                                                 ///< Dedicated event target.
    std::mutex _stoppingMutex;                                                     ///< Protects the stopping callback.
    std::function<void()> _completeStoppingFn;                                     ///< Internal loop-quit callback.
};

}
