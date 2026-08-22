// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1RequestDecoder.hpp"
#include "Http1RequestEncoder.hpp"
#include "Http1ResponseDecoder.hpp"
#include "Http1ResponseEncoder.hpp"
#include "Http1Transaction_fwd.hpp"
#include "Http1TransactionCallbacks.hpp"
#include "Http1TransactionOptions.hpp"

#include "../../../../mem/RingBuffer.hpp"
#include "../../../../time/TimePoint.hpp"
#include "../../../source/Connection.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace erbsland::network::impl {

/// One owner-loop HTTP/1 request/response transaction over an active connection.
/// @tested{Http1TransactionTest}
class Http1Transaction final : public std::enable_shared_from_this<Http1Transaction> {
public:
    /// The symmetric transaction role.
    enum class Role : std::uint8_t {
        Client, ///< Send one request and receive one final response.
        Server, ///< Receive one request and send one final response.
    };
    /// The owner-loop transaction lifecycle state.
    enum class State : std::uint8_t {
        Inactive,           ///< Callbacks may be configured before start.
        ReceivingHeaders,   ///< Waiting for the incoming final message head.
        AwaitingBodyPolicy, ///< Input is paused at a body-bearing head checkpoint.
        ReceivingBody,      ///< Streaming or aggregating incoming body data.
        Closing,            ///< A non-reusable transaction is closing its connection.
        Completed,          ///< Successful terminal transaction state.
        Failed,             ///< Terminal operational failure.
        Cancelled,          ///< Explicitly cancelled terminal state.
    };

private:
    /// The selected incoming-body delivery policy.
    enum class BodyMode : std::uint8_t {
        None,
        Awaiting,
        Streaming,
        Aggregating,
        Rejected,
    };

public: // factories
    /// Create an inactive client transaction.
    /// @param connection The active exclusive application stream.
    /// @param request The request head to serialize.
    /// @param options Captured transaction limits and deadlines.
    [[nodiscard]] static auto createClient(
        ConnectionPtr connection, HttpRequestHead request, Http1TransactionOptions options = {}) -> Http1TransactionPtr;
    /// Create an inactive server transaction.
    /// @param connection The active exclusive application stream.
    /// @param options Captured transaction limits and deadlines.
    [[nodiscard]] static auto createServer(ConnectionPtr connection, Http1TransactionOptions options = {})
        -> Http1TransactionPtr;

public:
    // defaults/deletions
    ~Http1Transaction() = default;
    Http1Transaction(const Http1Transaction &) = delete;
    Http1Transaction(Http1Transaction &&) = delete;
    auto operator=(const Http1Transaction &) -> Http1Transaction & = delete;
    auto operator=(Http1Transaction &&) -> Http1Transaction & = delete;

public: // configuration/start
    /// Access callbacks before or during the owner-loop transaction.
    [[nodiscard]] auto callbacks() noexcept -> Http1TransactionCallbacks & { return _callbacks; }
    /// Bind connection callbacks, start deadlines, and begin protocol processing.
    void start();

public: // incoming body policy
    /// Select incremental delivery at the current body checkpoint.
    void streamBody();
    /// Select bounded aggregation at the current body checkpoint.
    /// @param maximumLength The finite aggregate maximum.
    void aggregateBody(unit::ByteLength maximumLength);
    /// Reject the remaining incoming body and require connection closure.
    void rejectBody();
    /// Pause an active streaming body.
    void pauseBody();
    /// Resume an explicitly paused streaming body.
    void resumeBody();

public: // outgoing message
    /// Start the server's one final response.
    /// @param response The final response head.
    void startResponse(HttpResponseHead response);
    /// Queue one informational server response.
    /// @param response A non-switching informational response.
    /// @return Whether the complete semantic operation was accepted.
    [[nodiscard]] auto sendInformationalResponse(HttpResponseHead response) -> NetworkSendStatus;
    /// Atomically accept one outgoing semantic body block.
    [[nodiscard]] auto sendBody(const mem::ByteBlock &data) -> NetworkSendStatus;
    /// Finish the outgoing message with optional trailers.
    [[nodiscard]] auto finishBody(HttpHeaders trailers = {}) -> NetworkSendStatus;

public: // lifecycle/status
    /// Cancel immediately and abort the underlying connection.
    void cancel() noexcept;
    /// Get the transaction role.
    [[nodiscard]] auto role() const noexcept -> Role { return _role; }
    /// Get the current state.
    [[nodiscard]] auto state() const noexcept -> State { return _state; }
    /// Test whether completed framing permits connection reuse.
    [[nodiscard]] auto isConnectionReusable() const noexcept -> bool { return _reusable; }
    /// Detach bounded bytes belonging to the next pipelined message.
    [[nodiscard]] auto takeBufferedInput() -> mem::ByteBlock;

private:
    /// Create a transaction with a validated role-specific codec set.
    Http1Transaction(
        Role role, ConnectionPtr connection, Http1TransactionOptions options, std::optional<HttpRequestHead> request);

private: // connection callbacks
    /// Install common callbacks on the exclusive connection.
    void bindConnection();
    /// Clear callbacks owned by this transaction.
    void unbindConnection();
    /// Process one received connection block.
    void handleData(mem::ByteBlock data);
    /// Retry connection output and report semantic writability.
    void handleWritable();
    /// Translate orderly connection EOF into HTTP framing.
    void handleClosed();
    /// Translate an underlying connection failure.
    void handleError(const NetworkErrorContext &context);
    /// Observe underlying finalization.
    void handleConnectionFinal();

private: // decoding
    /// Feed pending input and poll all currently available decode events.
    void drainInput();
    /// Process one decoded event.
    void handleDecodeEvent(Http1DecodeEvent event);
    /// Process one request or final response head checkpoint.
    void handleFinalHead(const Http1DecodeEvent &event);
    /// Test whether a decoded head has a semantic body.
    [[nodiscard]] static auto hasBody(const Http1DecodeEvent &event) noexcept -> bool;
    /// Return the active decoder's next event.
    [[nodiscard]] auto nextDecodeEvent() -> std::optional<Http1DecodeEvent>;
    /// Feed the active decoder.
    [[nodiscard]] auto feedDecoder(mem::ConstByteSpan data) -> NetworkSendStatus;
    /// Signal EOF to the active decoder.
    void endDecoderInput();
    /// Reset an informational response decoder.
    void resetResponseDecoder();

private: // encoding
    /// Access the role's final-message encoder.
    [[nodiscard]] auto finalEncoderQueuedLength() const noexcept -> unit::ByteLength;
    /// Test whether the final encoder accepted all semantic input.
    [[nodiscard]] auto isFinalEncoderComplete() const noexcept -> bool;
    /// Take output from the active informational or final encoder.
    [[nodiscard]] auto takeEncoderOutput() -> mem::ByteBlock;
    /// Move bounded encoded blocks into the connection.
    void pumpOutput();
    /// Update whether final output was completely handed to the connection.
    void updateOutgoingDrained();

private: // lifecycle and timing
    /// Validate captured options and active connection state.
    void validateStart() const;
    /// Mark decoded body progress and schedule its rolling idle timer.
    void recordBodyProgress();
    /// Schedule the absolute header and total deadlines.
    void startInitialTimers();
    /// Enter graceful connection closure with an independent deadline.
    void startClosing();
    /// Handle a generation-checked header timeout.
    void handleHeaderTimeout(std::uint64_t generation);
    /// Handle a generation-checked body-idle timeout.
    void handleBodyTimeout(std::uint64_t generation, time::TimePoint deadline);
    /// Handle a generation-checked total timeout.
    void handleTotalTimeout(std::uint64_t generation);
    /// Handle a generation-checked graceful-close timeout.
    void handleCloseTimeout(std::uint64_t generation);
    /// Recompute successful transaction completion.
    void assessCompletion();
    /// Emit successful completion once and either detach or close.
    void finishSuccess();
    /// Emit one failure and select recoverable server handling or immediate abort.
    void fail(Http1TransactionFailure failure, bool serverCanRespond);
    /// Emit finalization once.
    void finishFinal();
    /// Mark the connection non-reusable from a message head.
    void assessPersistence(const Http1DecodeEvent &event);
    /// Test whether Connection fields conservatively require closure.
    [[nodiscard]] static auto requestsConnectionClose(const HttpHeaders &headers) -> bool;

private:
    Role _role;                                                  ///< Client or server transaction role.
    ConnectionPtr _connection;                                   ///< Exclusive active byte-stream connection.
    Http1TransactionOptions _options;                            ///< Captured limits and deadlines.
    Http1TransactionCallbacks _callbacks;                        ///< Owner-loop callbacks.
    std::unique_ptr<Http1RequestDecoder> _requestDecoder;        ///< Server request decoder.
    std::unique_ptr<Http1ResponseDecoder> _responseDecoder;      ///< Client response decoder.
    std::unique_ptr<Http1RequestEncoder> _requestEncoder;        ///< Client request encoder.
    std::unique_ptr<Http1ResponseEncoder> _responseEncoder;      ///< Server final response encoder.
    std::unique_ptr<Http1ResponseEncoder> _informationalEncoder; ///< Pending informational response.
    std::unique_ptr<mem::RingBuffer> _aggregate;                 ///< Optional bounded body aggregation.
    std::optional<mem::ByteBlock> _pendingInput;                 ///< One connection block awaiting decoder capacity.
    std::optional<mem::ByteBlock> _pendingOutput;                ///< One encoded block rejected by the connection.
    mem::ByteBlock _bufferedInput;                          ///< Bounded input retained for a sequential transaction.
    HttpMethod _requestMethod;                              ///< Request context for response framing.
    State _state{State::Inactive};                          ///< Current transaction state.
    BodyMode _bodyMode{BodyMode::None};                     ///< Incoming body delivery policy.
    std::optional<unit::ByteLength> _incomingContentLength; ///< Fixed incoming body length at the checkpoint.
    time::TimePoint _bodyDeadline;                          ///< Rolling decoded-body deadline.
    std::uint64_t _timerGeneration{};                       ///< Invalidates every pending transaction timer.
    bool _streamPaused{};                                   ///< Explicit streaming pause.
    bool _drainingInput{};                                  ///< Decoder drain reentrancy guard.
    bool _drainInputAgain{};                                ///< A nested operation requested another drain.
    bool _informationalInput{};                             ///< Current decoded response is informational.
    bool _incomingComplete{};                               ///< Final incoming message completed.
    bool _outgoingStarted{};                                ///< Final outgoing message exists.
    bool _outgoingTerminated{};                             ///< An early peer response terminated local upload.
    bool _outgoingDrained{};                                ///< Final outgoing bytes reached the connection queue.
    bool _semanticBlocked{};                                ///< A producer is owed one writable event.
    bool _reusable{true};                                   ///< Current persistence eligibility.
    bool _failureEmitted{};                                 ///< Exactly-once failure guard.
    bool _completionEmitted{};                              ///< Exactly-once completion guard.
    bool _finalized{};                                      ///< Exactly-once final guard.
};

}
