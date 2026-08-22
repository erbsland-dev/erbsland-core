// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Http1Transaction.hpp"

#include "Http1ProtocolError.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../text/Literals.hpp"
#include "../../ConnectionProtocolAccess.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

auto Http1Transaction::createClient(ConnectionPtr connection, HttpRequestHead request, Http1TransactionOptions options)
    -> Http1TransactionPtr {
    return Http1TransactionPtr{new Http1Transaction{Role::Client, std::move(connection), options, std::move(request)}};
}

auto Http1Transaction::createServer(ConnectionPtr connection, Http1TransactionOptions options) -> Http1TransactionPtr {
    return Http1TransactionPtr{new Http1Transaction{Role::Server, std::move(connection), options, {}}};
}

Http1Transaction::Http1Transaction(
    const Role role,
    ConnectionPtr connection,
    const Http1TransactionOptions options,
    std::optional<HttpRequestHead> request) :
    _role{role}, _connection{std::move(connection)}, _options{options} {
    if (!_connection) {
        throw err::ParameterError{"A connection is required for an HTTP transaction."_el, "connection"_el};
    }
    if (_role == Role::Client) {
        if (!request || !request->isValid()) {
            throw err::ParameterError{"A valid request is required for a client transaction."_el, "request"_el};
        }
        _requestMethod = request->method();
        _responseDecoder = std::make_unique<Http1ResponseDecoder>(_requestMethod, _options.codecLimits());
        _requestEncoder = std::make_unique<Http1RequestEncoder>(std::move(*request), _options.codecLimits());
        _outgoingStarted = true;
    } else {
        _requestDecoder = std::make_unique<Http1RequestDecoder>(_options.codecLimits());
    }
}

void Http1Transaction::start() {
    if (_state != State::Inactive) {
        throw err::LogicError{"An HTTP transaction can only be started once."_el};
    }
    validateStart();
    if (!ConnectionProtocolAccess::claim(*_connection, shared_from_this())) {
        throw err::LogicError{"The connection already has an active protocol transaction."_el};
    }
    auto retainedInput = ConnectionProtocolAccess::takeRetainedInput(*_connection);
    if (!retainedInput.isEmpty()) {
        _pendingInput = std::move(retainedInput);
    }
    bindConnection();
    _state = State::ReceivingHeaders;
    startInitialTimers();
    pumpOutput();
    updateOutgoingDrained();
    drainInput();
}

void Http1Transaction::streamBody() {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (_state != State::AwaitingBodyPolicy || _bodyMode != BodyMode::Awaiting) {
        throw err::LogicError{"No incoming HTTP body is awaiting a delivery policy."_el};
    }
    _bodyMode = BodyMode::Streaming;
    _state = State::ReceivingBody;
    _streamPaused = false;
    recordBodyProgress();
    _connection->resumeReceiving();
    drainInput();
}

void Http1Transaction::aggregateBody(const unit::ByteLength maximumLength) {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (_state != State::AwaitingBodyPolicy || _bodyMode != BodyMode::Awaiting) {
        throw err::LogicError{"No incoming HTTP body is awaiting a delivery policy."_el};
    }
    if (maximumLength.isInfinite()) {
        throw err::ParameterError{"The HTTP aggregate body limit must be finite."_el, "maximumLength"_el};
    }
    if (_incomingContentLength && *_incomingContentLength > maximumLength) {
        fail(
            Http1TransactionFailure{
                Http1TransactionFailure::Kind::ResourceLimit,
                Http1TransactionFailure::Phase::Body,
                "The declared HTTP body exceeds its selected aggregate limit."_el,
                Http1FailureReason::BodyLimitExceeded},
            _role == Role::Server);
        return;
    }
    _aggregate =
        std::make_unique<mem::RingBuffer>(Http1CodecLimits::initialQueueCapacity(maximumLength), maximumLength);
    _bodyMode = BodyMode::Aggregating;
    _state = State::ReceivingBody;
    recordBodyProgress();
    _connection->resumeReceiving();
    drainInput();
}

void Http1Transaction::rejectBody() {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (_state != State::AwaitingBodyPolicy || _bodyMode != BodyMode::Awaiting) {
        throw err::LogicError{"No incoming HTTP body is awaiting rejection."_el};
    }
    _bodyMode = BodyMode::Rejected;
    _reusable = false;
    _connection->pauseReceiving();
    fail(
        Http1TransactionFailure{
            Http1TransactionFailure::Kind::ResourceLimit,
            Http1TransactionFailure::Phase::Body,
            "The incoming HTTP body was rejected."_el},
        _role == Role::Server);
}

void Http1Transaction::pauseBody() {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (_bodyMode != BodyMode::Streaming || _state != State::ReceivingBody || _streamPaused) {
        throw err::LogicError{"The HTTP streaming body is not active."_el};
    }
    _streamPaused = true;
    _connection->pauseReceiving();
}

void Http1Transaction::resumeBody() {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (_bodyMode != BodyMode::Streaming || _state != State::ReceivingBody || !_streamPaused) {
        throw err::LogicError{"The HTTP streaming body is not paused."_el};
    }
    _streamPaused = false;
    recordBodyProgress();
    _connection->resumeReceiving();
    drainInput();
}

void Http1Transaction::startResponse(HttpResponseHead response) {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (_role != Role::Server || _outgoingStarted || !response.isValid() || response.status().isInformational()) {
        throw err::LogicError{"A valid final server response can only be started once."_el};
    }
    if (response.status() == HttpStatus::SwitchingProtocols ||
        (_requestMethod.standardType() == HttpMethodType::Connect && response.status().isSuccessful())) {
        fail(
            Http1TransactionFailure{
                Http1TransactionFailure::Kind::UnsupportedSwitch,
                Http1TransactionFailure::Phase::Headers,
                "HTTP protocol switching is not supported by this transaction engine."_el},
            false);
        return;
    }
    if (!_incomingComplete && _bodyMode != BodyMode::None) {
        _bodyMode = BodyMode::Rejected;
        _incomingComplete = true;
        _reusable = false;
        _connection->pauseReceiving();
    }
    if (response.version() == HttpVersion::Http10 || requestsConnectionClose(response.headers())) {
        _reusable = false;
    }
    _responseEncoder =
        std::make_unique<Http1ResponseEncoder>(std::move(response), _requestMethod, _options.codecLimits());
    _outgoingStarted = true;
    pumpOutput();
    updateOutgoingDrained();
    assessCompletion();
}

auto Http1Transaction::sendInformationalResponse(HttpResponseHead response) -> NetworkSendStatus {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (_role != Role::Server || _outgoingStarted || _informationalEncoder || !response.isValid() ||
        !response.status().isInformational() || response.status() == HttpStatus::SwitchingProtocols) {
        return NetworkSendStatus::Closed;
    }
    _informationalEncoder =
        std::make_unique<Http1ResponseEncoder>(std::move(response), _requestMethod, _options.codecLimits());
    const auto result = _informationalEncoder->finish();
    if (!result.isAccepted()) {
        _informationalEncoder.reset();
        _semanticBlocked = result.wouldBlock();
        return result;
    }
    pumpOutput();
    return NetworkSendStatus::Accepted;
}

auto Http1Transaction::sendBody(const mem::ByteBlock &data) -> NetworkSendStatus {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (!_outgoingStarted || _outgoingTerminated || _informationalEncoder) {
        return NetworkSendStatus::Closed;
    }
    auto result = NetworkSendStatus::Closed;
    if (_role == Role::Client && _requestEncoder) {
        result = _requestEncoder->writeBody(data.span());
    } else if (_role == Role::Server && _responseEncoder) {
        result = _responseEncoder->writeBody(data.span());
    }
    if (result.wouldBlock()) {
        _semanticBlocked = true;
        pumpOutput();
        return result;
    }
    if (result.isAccepted()) {
        pumpOutput();
        updateOutgoingDrained();
    }
    return result;
}

auto Http1Transaction::finishBody(HttpHeaders trailers) -> NetworkSendStatus {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (!_outgoingStarted || _outgoingTerminated || _informationalEncoder) {
        return NetworkSendStatus::Closed;
    }
    auto result = NetworkSendStatus::Closed;
    if (_role == Role::Client && _requestEncoder) {
        result = _requestEncoder->finish(std::move(trailers));
    } else if (_role == Role::Server && _responseEncoder) {
        result = _responseEncoder->finish(std::move(trailers));
    }
    if (result.wouldBlock()) {
        _semanticBlocked = true;
        pumpOutput();
        return result;
    }
    if (result.isAccepted()) {
        pumpOutput();
        updateOutgoingDrained();
        assessCompletion();
    }
    return result;
}

void Http1Transaction::cancel() noexcept {
    if (_state == State::Completed || _state == State::Cancelled || _finalized) {
        return;
    }
    _state = State::Cancelled;
    _reusable = false;
    ++_timerGeneration;
    _connection->abort();
    finishFinal();
}

auto Http1Transaction::takeBufferedInput() -> mem::ByteBlock {
    ConnectionProtocolAccess::verifyOwner(*_connection);
    if (!_finalized || !_reusable) {
        throw err::LogicError{"Buffered HTTP input is only available after reusable completion."_el};
    }
    return ConnectionProtocolAccess::takeRetainedInput(*_connection);
}

}
