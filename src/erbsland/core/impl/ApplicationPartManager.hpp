// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPartRecord.hpp"

#include "../ApplicationPartManager.hpp"

#include "../../event/EventThread_fwd.hpp"
#include "../../event/impl/EventCallbackList.hpp"
#include "../../text/StringHashMap.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace erbsland::core::impl {

/// Default detached application-part manager implementation.
/// @tested{ApplicationPartManagerTest ApplicationPartApplicationTest}
class ApplicationPartManager final : public core::ApplicationPartManager,
                                     private core::ApplicationPartManagerEventEditor,
                                     public std::enable_shared_from_this<ApplicationPartManager> {
public:
    /// Create a manager with safe destruction from owned callback threads.
    [[nodiscard]] static auto create(event::EventsPtr controlEvents) -> std::shared_ptr<ApplicationPartManager>;

public:
    /// Create a manager with an optional external control event target.
    explicit ApplicationPartManager(event::EventsPtr controlEvents);
    /// Stop owned threads and release the manager.
    ~ApplicationPartManager() noexcept override;

    // defaults/deletions
    ApplicationPartManager(const ApplicationPartManager &) = delete;
    auto operator=(const ApplicationPartManager &) -> ApplicationPartManager & = delete;
    ApplicationPartManager(ApplicationPartManager &&) = delete;
    auto operator=(ApplicationPartManager &&) -> ApplicationPartManager & = delete;

public:
    /// Create and start the private control thread when no target was supplied.
    void initializeControlThread();

public: // implement core::ApplicationPartManagerAccess
    [[nodiscard]] auto state() const noexcept -> ApplicationPartManagerState override;
    [[nodiscard]] auto partState(const ApplicationPartIdentifierPtr &identifier) const -> ApplicationPartState override;
    [[nodiscard]] auto waitForRunning() -> bool override;
    [[nodiscard]] auto waitForStopped() -> bool override;
    [[nodiscard]] auto waitForRunning(const ApplicationPartIdentifierPtr &identifier) -> bool override;
    [[nodiscard]] auto waitForStopped(const ApplicationPartIdentifierPtr &identifier) -> bool override;
    [[nodiscard]] auto part(const ApplicationPartIdentifierPtr &identifier) const -> ApplicationPartPtr override;

public: // implement core::ApplicationPartManager
    void prepare() override;
    void registerCommandLineOptions(const options::OptionsPtr &options) override;
    void parseCommandLine(const options::OptionValuesPtr &values) override;
    void start() override;
    void start(const ApplicationPartIdentifierPtr &identifier) override;
    void stop(const ApplicationPartIdentifierPtr &identifier) override;
    void stop() override;
    [[nodiscard]] auto events() noexcept -> core::ApplicationPartManagerEventEditor & override;
    void setErrorHandler(ApplicationPartErrorHandler handler) override;

private: // implement core::ApplicationPartManagerEventEditor
    [[nodiscard]] auto addStateChanged(ApplicationPartManagerStateChangedFn callback)
        -> event::EventSubscription override;
    [[nodiscard]] auto addPartStateChanged(ApplicationPartStateChangedFn callback) -> event::EventSubscription override;

public: // errors
    [[nodiscard]] auto hasError() const noexcept -> bool override;
    [[nodiscard]] auto takeError() noexcept -> std::exception_ptr override;

protected:
    void registerPart(Registration registration) override;

private: // preparation
    /// Resolve an identifier under the manager lock.
    [[nodiscard]] auto resolveIdentifier(const ApplicationPartIdentifierPtr &identifier) const -> std::size_t;
    /// Resolve an identifier while the caller holds the manager lock.
    [[nodiscard]] auto resolveIdentifierLocked(const ApplicationPartIdentifierPtr &identifier) const -> std::size_t;
    /// Reject cycles in the prepared dependency graph.
    void validateAcyclicGraph() const;
    /// Visit one part while validating the dependency graph.
    void validateAcyclicPart(std::size_t number, std::vector<uint8_t> &marks) const;

private: // control dispatch
    /// Delete a manager directly or transfer deletion away from an owned thread.
    static void destroy(ApplicationPartManager *manager) noexcept;
    /// Test whether the current thread is owned by this manager.
    [[nodiscard]] auto isCurrentThreadOwned() const noexcept -> bool;
    /// Queue serialized work on the control event source.
    void invokeControl(std::function<void()> callback);
    /// Remember the thread that dispatches control callbacks.
    void recordControlThread() noexcept;
    /// Test whether waiting on the current thread would deadlock.
    [[nodiscard]] auto isWaitForbiddenLocked() const noexcept -> bool;

private: // startup
    /// Begin the initial automatic startup phase.
    void handleStartAll();
    /// Handle one manual part-start request.
    void handleStartPart(std::size_t number);
    /// Reject a manual start whose dependency closure is terminal.
    void validateDependencyClosureCanStartLocked(std::size_t number, std::vector<uint8_t> &marks) const;
    /// Mark one complete dependency closure for manual startup.
    void requestDependencyClosure(std::size_t number);
    /// Start every currently eligible requested or automatic part.
    void progressStartup();
    /// Test whether all dependencies of a part are running.
    [[nodiscard]] auto dependenciesRunning(const ApplicationPartRecord &record) const noexcept -> bool;
    /// Create and start the dedicated thread for one part.
    void startPart(std::size_t number);
    /// Publish successful startup from a part thread.
    void handlePartRunning(std::size_t number);

private: // stopping
    /// Begin complete reverse-dependency shutdown.
    void handleStopAll();
    /// Handle one manual part-stop request.
    void handleStopPart(std::size_t number);
    /// Mark one complete dependent closure for shutdown.
    void requestDependentClosure(std::size_t number);
    /// Stop every requested part whose dependents are terminal.
    void progressStopping();
    /// Test whether every dependent of a part is terminal.
    [[nodiscard]] auto dependentsTerminal(const ApplicationPartRecord &record) const noexcept -> bool;
    /// Queue the stopping hook and arm the shutdown timeout for one part.
    void stopPart(std::size_t number);
    /// Join a completed part thread and publish its terminal state.
    void handlePartExited(std::size_t number);
    /// Force loop shutdown after a part exceeds its timeout.
    void handlePartTimeout(std::size_t number, uint64_t generation);

private: // errors and state
    /// Store and route one lifecycle or event callback failure.
    void handlePartFailure(std::size_t number, std::exception_ptr error);
    /// Store a user callback failure and force complete shutdown.
    void handleCallbackFailure(std::exception_ptr error);
    /// Append a non-null exception to the ordered error queue.
    void addError(std::exception_ptr error) noexcept;
    /// Publish a manager state and dispatch its observers.
    void setManagerState(ApplicationPartManagerState state);
    /// Publish one part state and dispatch its observer.
    void setPartState(std::size_t number, ApplicationPartState state);
    /// Publish the manager terminal state after all parts are terminal.
    void finishManagerIfStopped();

private:
    const uint64_t _managerToken;          ///< Unique token used by identifier caches.
    mutable std::mutex _mutex;             ///< Protects all observable manager state.
    std::condition_variable _stateChanged; ///< Wakes state waiters.
    ApplicationPartManagerState _state{ApplicationPartManagerState::Uninitialized}; ///< Manager state.
    std::vector<Registration> _registrations;                                       ///< Deferred registrations.
    std::vector<ApplicationPartRecord> _records;    ///< Prepared parts, number minus one indexed.
    text::StringHashMap<std::size_t> _nameToNumber; ///< Stable name to manager-local number.
    std::deque<std::exception_ptr> _errors;         ///< Ordered manager errors.
    ApplicationPartErrorHandler _errorHandler;      ///< Part failure policy.
    event::impl::EventCallbackList<ApplicationPartManagerStateChangedFn> _stateChangedCallbacks; ///< State observers.
    event::impl::EventCallbackList<ApplicationPartStateChangedFn> _partStateChangedCallbacks;    ///< Part observers.
    event::EventsPtr _controlEvents;                    ///< Serializes runtime graph changes.
    event::UnmanagedEventThreadPtr _ownedControlThread; ///< Optional private control thread.
    std::thread::id _controlThreadId;                   ///< Control thread observed during dispatch.
    bool _prepared{false};                              ///< True after successful preparation.
    bool _stopAllRequested{false};                      ///< True during complete manager shutdown.
    bool _fatalFailure{false};                          ///< Selects the final `Failed` state.
};

}
