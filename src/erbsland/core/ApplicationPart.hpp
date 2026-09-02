// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPart_fwd.hpp"
#include "ApplicationPartCommandLine.hpp"
#include "ApplicationPartIdentifier_fwd.hpp"
#include "ApplicationPartManager_fwd.hpp"
#include "ApplicationPartState.hpp"

#include "impl/ApplicationPartManager_fwd.hpp"

#include "../event/Events_fwd.hpp"
#include "../time/TimeDelta.hpp"
#include "../util/List.hpp"

#include <atomic>
#include <functional>
#include <mutex>

namespace erbsland::core {

/// Base class for an independently managed application component.
/// Lifecycle hooks run on the part's dedicated event thread except `automaticStart()`, which runs on the manager
/// control event source, and the command-line hooks, which run synchronously on their caller's thread.
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
    /// @throws err::LogicError If this part was not prepared by a manager.
    [[nodiscard]] auto partManager() const -> ApplicationPartManagerAccess &;
    /// Access this part's event target.
    /// @throws err::LogicError If the part has not started yet.
    [[nodiscard]] auto events() const -> event::EventsPtr;

public: // implement ApplicationPartCommandLine
    void registerCommandLineOptions(const options::OptionsPtr &options) override;
    void parseCommandLine(const options::OptionValuesPtr &values) override;

protected: // lifecycle customization
    /// Decide whether this part starts during initial automatic startup.
    [[nodiscard]] virtual auto automaticStart() -> bool;
    /// Get the maximum graceful-shutdown time before the event loop is forced to quit.
    [[nodiscard]] virtual auto shutdownTimeout() const noexcept -> time::TimeDelta;
    /// Initialize this part on its dedicated event thread.
    virtual void initialize();
    /// Notify this part that startup completed on its dedicated event thread.
    virtual void running();
    /// Begin shutdown on this part's dedicated event thread.
    /// The default implementation calls `completeStopping()` immediately.
    virtual void stopping();
    /// Clean up after the event loop has exited, on the dedicated part thread.
    virtual void cleanup() noexcept;
    /// Complete asynchronous stopping and request the part event loop to quit.
    void completeStopping() noexcept;

private: // manager access
    /// Bind this part to its prepared manager and optional runtime event target.
    void bind(ApplicationPartManagerAccessWeakPtr manager, event::EventsPtr events);
    /// Publish a manager-owned lifecycle state.
    void setState(ApplicationPartState state) noexcept;

private:
    std::atomic<ApplicationPartState> _state{ApplicationPartState::Uninitialized}; ///< Current lifecycle state.
    ApplicationPartManagerAccessWeakPtr _manager;                                  ///< Owning manager access.
    event::EventsPtr _events;                                                      ///< Dedicated event target.
    std::mutex _stoppingMutex;                                                     ///< Protects the stopping callback.
    std::function<void()> _completeStoppingFn;                                     ///< Internal loop-quit callback.
};

}
