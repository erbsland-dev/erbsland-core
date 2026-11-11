// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SubprocessBackend.hpp"

#include "../../../event/EventBackendTarget_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../mem/RingBuffer.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../SubprocessEventEditor.hpp"
#include "../../SubprocessOptions.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

namespace erbsland::system::impl {

/// Synchronized native process workflow and bounded event output.
/// @tested{SubprocessEventTest SubprocessInteropTest SubprocessFailureTest SubprocessOutputTest}
class SubprocessState final {
    /// Native workflow progression, independent of event dispatch.
    enum class Phase : uint8_t {
        Prepared,  ///< No launch has been attempted.
        Starting,  ///< Launch is claimed; native setup is in progress.
        Running,   ///< Native child exists and is monitored.
        Completed, ///< Native cleanup and output drainage have finished.
    };
    /// Whether future events are observed or permanently suppressed.
    enum class Observation : uint8_t {
        Unbound, ///< Future lifecycle events are not observed.
        Active,  ///< Future activity is routed to an owner loop.
        Closed,  ///< Source destruction suppresses callbacks.
    };
    /// Once-only overflow notification progression.
    enum class Overflow : uint8_t {
        None,      ///< No event output has been lost.
        Pending,   ///< First loss awaits notification.
        Delivered, ///< The once-only notification was claimed.
    };
    /// One callback and its output block, copied under the workflow lock.
    struct OutputDelivery final {
        SubprocessEventEditor::OutputFn callback; ///< Current stream handler.
        text::String block;                       ///< Original bytes to deliver.
    };
    /// Terminal callbacks and data, atomically claimed after draining output.
    struct Completion final {
        SubprocessEventEditor::ExitFn exited;       ///< Ordinary outcome handler.
        SubprocessEventEditor::ErrorFn failed;      ///< Failure handler.
        SubprocessEventEditor::EventFn final;       ///< Final handler.
        PlatformErrorContextConstPtr error;         ///< Failure diagnostic.
        std::optional<SubprocessExitStatus> status; ///< Ordinary native outcome.
    };
    /// One independently routed captured stream.
    struct Output final {
        text::StringEditor captured;              ///< Retained procedural prefix.
        std::unique_ptr<mem::RingBuffer> pending; ///< Bounded event bytes.
        SubprocessEventEditor::OutputFn handler;  ///< Owner-loop callback.
        std::size_t bomPrefixLength{};            ///< Initial bytes matching the UTF-8 signature.
        bool bomResolved{};                       ///< Initial UTF-8 signature handling is complete.
        bool truncated{};                         ///< Either capture or event output overflowed.
        bool gap{};                               ///< Pending bytes end at a loss boundary.
    };

public:
    /// Injectable native launch boundary for workflow tests.
    using BackendFactory = std::function<decltype(SubprocessBackend::create)>;

public:
    /// Prepare launch storage.
    /// @param executable Direct executable path.
    /// @param arguments Launch arguments.
    /// @param options Child configuration.
    /// @param factory Native launch factory; defaults to the platform backend.
    SubprocessState(
        path::Path executable,
        text::StringList arguments,
        SubprocessOptions options,
        BackendFactory factory = SubprocessBackend::create);
    /// Join the workflow after stopping any remaining owned child.
    ~SubprocessState();

public: // accessors
    /// Get a synchronized status snapshot.
    auto status() const -> std::optional<SubprocessExitStatus>;
    /// Get the retained launch identifier.
    auto processId() const noexcept -> ProcessId;
    /// Test workflow activity.
    auto isRunning() const -> bool;
    /// Get retained output.
    /// @param error Whether to select stderr.
    auto captured(bool error) const -> text::String;
    /// Test whether a stream overflowed.
    /// @param error Whether to select stderr.
    auto truncated(bool error) const noexcept -> bool;

public: // lifecycle
    /// Start once and launch the workflow monitor.
    void start();
    /// Stop the owned child and join its monitor; suppress future event delivery.
    void shutdown() noexcept;

public: // process control
    /// Wait for native completion without delivering callbacks.
    /// @param timeout Non-negative wait budget for native exit and output drainage; zero polls, no value waits
    /// indefinitely.
    /// @return Status or no value on timeout.
    auto wait(std::optional<time::TimeDelta> timeout = {}) -> std::optional<SubprocessExitStatus>;
    /// Request native termination.
    /// @param force Whether to force termination.
    void terminate(bool force);

public: // event delivery
    /// Bind future notifications to an owner backend.
    /// @param target Backend wake target.
    void bind(event::EventBackendTargetWeakPtr target);
    /// Queue at most one owner-loop delivery callback when work is available.
    /// @param owner The bound callback target.
    /// @param self Weak workflow ownership for callback cancellation.
    void queueDispatch(const event::EventTargetPtr &owner, std::weak_ptr<SubprocessState> self);
    /// Deliver one bounded batch on the owner loop.
    void dispatch();

public: // output routing
    /// Change one output handler, discarding queued event bytes when cleared.
    /// @param error Whether to select stderr.
    /// @param callback The new callback.
    void setOutput(bool error, SubprocessEventEditor::OutputFn callback);

public: // handlers
    /// Replace the started handler.
    void setStarted(SubprocessEventEditor::EventFn callback);
    /// Replace the output truncation handler.
    void setOutputTruncated(SubprocessEventEditor::EventFn callback);
    /// Replace the exit handler.
    void setExit(SubprocessEventEditor::ExitFn callback);
    /// Replace the error handler.
    void setError(SubprocessEventEditor::ErrorFn callback);
    /// Replace the final handler.
    void setFinal(SubprocessEventEditor::EventFn callback);

private: // native workflow
    /// Claim the one-shot launch under the workflow lock.
    void beginLaunch();
    /// Create native resources and start the sole monitor.
    void launchNative();
    /// Publish the process identifier and successful launch notification.
    void recordLaunch(ProcessId id);
    /// Observe native completion independently of the owner event loop.
    void monitor() noexcept;
    /// Poll native completion without delaying while the native lock is held.
    auto pollNative() -> std::optional<SubprocessExitStatus>;
    /// Retain the native outcome and classify abnormal termination.
    void recordStatus(SubprocessExitStatus status);
    /// Retain the first failure for procedural exceptions and event delivery.
    void recordFailure(std::exception_ptr failure, PlatformErrorContextConstPtr error);
    /// Test for a recorded operational failure under the workflow lock.
    auto hasFailure() const -> bool;
    /// Record a native pipe read error for both API styles.
    /// @param error The native diagnostic snapshot.
    void readError(PlatformErrorContextConstPtr error);
    /// Stop and reap the child while preserving an existing workflow failure.
    void cleanupNative() noexcept;
    /// Atomically publish completion and its pending terminal notification.
    void markCompleted();
    /// Publish completion after native cleanup and wake both API styles.
    void complete();

private: // event delivery
    /// Permanently suppress new event dispatch after source destruction.
    void closeObservation();
    /// Test whether the owning source has been released.
    /// @return Whether further callbacks are suppressed.
    auto isClosed() const -> bool;
    /// Coalesce wakeups until the backend next polls this source.
    void wake() noexcept;
    /// Claim one coalesced backend wake under the workflow lock.
    auto claimWake() -> event::EventBackendTargetPtr;
    /// Atomically claim one queued callback if any event or output is ready.
    auto claimDispatch() -> bool;
    /// Release the queued-callback claim under the workflow lock.
    void releaseDispatch();
    /// Claim the pending launch notification and copy its current handler.
    auto takeStarted() -> SubprocessEventEditor::EventFn;
    /// Claim the once-only overflow notification and copy its current handler.
    auto takeOverflow() -> SubprocessEventEditor::EventFn;
    /// Claim terminal callbacks and snapshots after all queued output drains.
    auto takeCompletion() -> Completion;
    /// Invoke the terminal outcome then final callback without holding locks.
    void sendCompletion(const Completion &completion);

private: // output routing
    /// Route raw pipe bytes into capture or bounded event storage.
    /// @param error Whether the source is stderr.
    /// @param bytes Original native bytes.
    void receive(bool error, std::string_view bytes);
    /// Route incoming bytes under the workflow lock and account for overflow.
    void storeOutput(bool error, std::string_view bytes);
    /// Remove one initial UTF-8 signature across reads; the caller holds the workflow lock.
    auto removeOutputBom(Output &output, mem::ConstByteSpan bytes) -> mem::ConstByteSpan;
    /// Preserve an incomplete signature at mismatch or EOF; the caller holds the workflow lock.
    void flushOutputBomPrefix(Output &output);
    /// Store payload bytes and account for overflow; the caller holds the workflow lock.
    void storeOutput(Output &output, mem::ConstByteSpan bytes);
    /// Copy and consume one ready output block under the workflow lock.
    auto takeOutput(bool error) -> OutputDelivery;
    /// Deliver one bounded block without holding the workflow lock.
    /// @param error Whether to select stderr.
    void deliverOutput(bool error);
    /// Test whether output has a deliverable prefix; the caller holds the workflow lock.
    auto outputReady(Output &output) const -> bool;
    /// Find a UTF-8 safe prefix; the caller holds the workflow lock.
    auto deliverableLength(const Output &output, const text::String &pending) const -> unit::ByteCount;
    /// Copy at most one callback block from the pending ring.
    /// @param output The locked output state.
    /// @return Original pending bytes, limited by the configured block size.
    auto pendingText(Output &output) const -> text::String;
    /// Select a prefix without splitting a valid trailing UTF-8 sequence.
    /// @param bytes The original candidate bytes.
    /// @param maximum Maximum callback size.
    /// @param finished Whether these bytes end at EOF or an overflow gap.
    /// @return The deliverable prefix length.
    static auto chunkLength(const text::String &bytes, unit::ByteCount maximum, bool finished) -> unit::ByteCount;

private:
    path::Path _executable;                            ///< Prepared executable.
    text::StringList _arguments;                       ///< Prepared argument storage.
    SubprocessOptions _options;                        ///< Immutable launch options.
    mutable std::mutex _mutex;                         ///< Workflow, callbacks, and output lock.
    std::mutex _nativeMutex;                           ///< Serializes backend control and status operations.
    std::condition_variable _completedCondition;       ///< Procedural wait notification.
    BackendFactory _factory;                           ///< Injectable native launch boundary.
    SubprocessBackendPtr _backend;                     ///< Owned native process; accessed under the native lock.
    std::thread _monitor;                              ///< Sole native waiter.
    event::EventBackendTargetWeakPtr _target;          ///< Non-owning event wake destination.
    ProcessId _processId;                              ///< Immutable after launch.
    std::optional<SubprocessExitStatus> _status;       ///< Synchronized native exit status.
    std::exception_ptr _failure;                       ///< Procedural native failure.
    PlatformErrorContextConstPtr _error;               ///< Event diagnostic.
    Output _output;                                    ///< stdout route.
    Output _errorOutput;                               ///< stderr route.
    Phase _phase{Phase::Prepared};                     ///< Native workflow state.
    Observation _observation{Observation::Unbound};    ///< Event subscription lifetime.
    Overflow _overflow{Overflow::None};                ///< Once-only overflow state.
    SubprocessEventEditor::EventFn _onStarted;         ///< Successful launch handler.
    SubprocessEventEditor::EventFn _onOutputTruncated; ///< Overflow handler.
    SubprocessEventEditor::ExitFn _onExit;             ///< Ordinary outcome handler.
    SubprocessEventEditor::ErrorFn _onError;           ///< Failure handler.
    SubprocessEventEditor::EventFn _onFinal;           ///< Final handler.
    bool _startedPending{};                            ///< Successful launch notification remains pending.
    bool _terminalPending{};                           ///< Terminal and final notification remain pending.
    bool _deliveryQueued{};                            ///< At most one dispatch callback is queued.
    bool _wakePending{};                               ///< Native wakeups are coalesced until backend polling.
};

}
