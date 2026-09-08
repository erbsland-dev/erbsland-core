// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Http1Transaction.hpp"

#include "Http1ProtocolError.hpp"

#include "../../../../text/Literals.hpp"
#include "../../../source/NetworkErrorReason.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void Http1Transaction::bindConnection() {
    const auto weakSelf = Http1TransactionWeakPtr{shared_from_this()};
    _connection->events()
        .onData([weakSelf](mem::ByteBlock data) -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleData(std::move(data));
            }
        })
        .onWritable([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleWritable();
            }
        })
        .onClosed([weakSelf](const ConnectionCloseContext &) -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleClosed();
            }
        })
        .onError([weakSelf](const NetworkErrorContext &context) -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleError(context);
            }
        })
        .onFinal([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleConnectionFinal();
            }
        });
}

void Http1Transaction::unbindConnection() {
    _connection->events().onData({}).onWritable({}).onClosed({}).onError({}).onFinal({});
}

void Http1Transaction::handleData(mem::ByteBlock data) {
    if (_finalized || _state == State::Closing || _state == State::Cancelled || _failureEmitted) {
        return;
    }
    if (_pendingInput) {
        fail(
            Http1TransactionFailure{
                Http1TransactionFailure::Kind::ResourceLimit,
                Http1TransactionFailure::Phase::Body,
                "More connection input arrived while the HTTP decoder was back-pressured."_el,
                Http1FailureReason::InputLimitExceeded},
            _role == Role::Server);
        return;
    }
    _pendingInput = std::move(data);
    drainInput();
}

void Http1Transaction::handleWritable() {
    if (_finalized || _state == State::Cancelled) {
        return;
    }
    pumpOutput();
    updateOutgoingDrained();
    if (_semanticBlocked && !_pendingOutput) {
        _semanticBlocked = false;
        if (_callbacks.writable) {
            try {
                _callbacks.writable();
            } catch (...) {
                fail(
                    Http1TransactionFailure{
                        Http1TransactionFailure::Kind::Callback,
                        Http1TransactionFailure::Phase::Transport,
                        "The HTTP writable callback threw an exception."_el},
                    false);
                return;
            }
        }
    }
    assessCompletion();
}

void Http1Transaction::handleClosed() {
    if (_finalized || _state == State::Cancelled) {
        return;
    }
    if (_role == Role::Server && _state == State::ReceivingHeaders && !_incomingStarted) {
        _reusable = false;
        finishFinal();
        return;
    }
    if (!_incomingComplete && !_failureEmitted) {
        try {
            endDecoderInput();
            drainInput();
        } catch (const Http1ProtocolError &error) {
            fail(
                Http1TransactionFailure{
                    Http1TransactionFailure::Kind::Protocol,
                    _state == State::ReceivingHeaders ? Http1TransactionFailure::Phase::Headers
                                                      : Http1TransactionFailure::Phase::Body,
                    "HTTP decoding failed at connection EOF."_el,
                    error.reason()},
                _role == Role::Server);
        }
    }
    _reusable = false;
}

void Http1Transaction::handleError(const NetworkErrorContext &context) {
    // Browsers can discard an idle keep-alive connection without an orderly TLS or TCP shutdown. Once request data
    // arrived, the same errors remain failures until the complete request has been decoded.
    const auto noIncompleteRequest = (_state == State::ReceivingHeaders && !_incomingStarted) || _incomingComplete;
    const auto normalPeerDisconnect = _role == Role::Server && noIncompleteRequest &&
        (context.reason() == NetworkErrorReason::TlsTruncation ||
            context.reason() == NetworkErrorReason::ConnectionReset);
    if (!_finalized && !_failureEmitted && normalPeerDisconnect) {
        _reusable = false;
        finishFinal();
        return;
    }
    if (!_finalized && !_failureEmitted) {
        fail(Http1TransactionFailure{context}, false);
    }
}

void Http1Transaction::handleConnectionFinal() {
    if (_finalized) {
        return;
    }
    if (_state == State::Closing || _failureEmitted || _state == State::Cancelled) {
        finishFinal();
        return;
    }
    fail(
        Http1TransactionFailure{
            Http1TransactionFailure::Kind::Transport,
            Http1TransactionFailure::Phase::Transport,
            "The connection finalized before the HTTP transaction completed."_el},
        false);
}

void Http1Transaction::drainInput() {
    if (_drainingInput) {
        _drainInputAgain = true;
        return;
    }
    _drainingInput = true;
    do {
        _drainInputAgain = false;
        try {
            if (_pendingInput) {
                const auto result = feedDecoder(_pendingInput->span());
                if (result.isAccepted()) {
                    _pendingInput.reset();
                } else if (result.wouldBlock()) {
                    _connection->pauseReceiving();
                } else {
                    fail(
                        Http1TransactionFailure{
                            Http1TransactionFailure::Kind::Protocol,
                            Http1TransactionFailure::Phase::Body,
                            "The HTTP decoder no longer accepts connection input."_el,
                            Http1FailureReason::InvalidState},
                        _role == Role::Server);
                }
            }
            while (
                !_failureEmitted && _state != State::AwaitingBodyPolicy && _bodyMode != BodyMode::Rejected &&
                !_streamPaused) {
                auto event = nextDecodeEvent();
                if (!event) {
                    break;
                }
                handleDecodeEvent(std::move(*event));
            }
        } catch (const Http1ProtocolError &error) {
            const auto resource = error.reason() == Http1FailureReason::StartLineTooLong ||
                error.reason() == Http1FailureReason::HeaderLimitExceeded ||
                error.reason() == Http1FailureReason::TrailerLimitExceeded ||
                error.reason() == Http1FailureReason::BodyLimitExceeded ||
                error.reason() == Http1FailureReason::InputLimitExceeded;
            fail(
                Http1TransactionFailure{
                    resource ? Http1TransactionFailure::Kind::ResourceLimit : Http1TransactionFailure::Kind::Protocol,
                    _state == State::ReceivingHeaders ? Http1TransactionFailure::Phase::Headers
                                                      : Http1TransactionFailure::Phase::Body,
                    "HTTP decoding failed."_el,
                    error.reason()},
                _role == Role::Server);
        }
    } while (_drainInputAgain && !_failureEmitted);
    _drainingInput = false;
    if (!_pendingInput && _state == State::ReceivingBody && !_streamPaused) {
        _connection->resumeReceiving();
    }
}

void Http1Transaction::handleDecodeEvent(Http1DecodeEvent event) {
    switch (event.kind()) {
    case Http1DecodeEvent::Kind::RequestHead:
        _requestMethod = event.request().method();
        handleFinalHead(event);
        break;
    case Http1DecodeEvent::Kind::ResponseHead:
        if (event.response().status().isInformational()) {
            if (event.response().status() == HttpStatus::SwitchingProtocols) {
                fail(
                    Http1TransactionFailure{
                        Http1TransactionFailure::Kind::UnsupportedSwitch,
                        Http1TransactionFailure::Phase::Headers,
                        "HTTP protocol switching is not supported by this transaction engine."_el},
                    false);
                return;
            }
            _informationalInput = true;
            if (_callbacks.informationalHead) {
                try {
                    _callbacks.informationalHead(event);
                } catch (...) {
                    fail(
                        Http1TransactionFailure{
                            Http1TransactionFailure::Kind::Callback,
                            Http1TransactionFailure::Phase::Headers,
                            "The HTTP informational callback threw an exception."_el},
                        false);
                }
            }
        } else {
            const auto isConnect = _requestMethod.standardType() == HttpMethodType::Connect;
            if (isConnect && event.response().status().isSuccessful()) {
                fail(
                    Http1TransactionFailure{
                        Http1TransactionFailure::Kind::UnsupportedSwitch,
                        Http1TransactionFailure::Phase::Headers,
                        "Successful CONNECT protocol switching is not supported."_el},
                    false);
                return;
            }
            if (_requestEncoder && !_requestEncoder->isComplete()) {
                _outgoingTerminated = true;
                _requestEncoder.reset();
                _pendingOutput.reset();
                _outgoingDrained = true;
                _reusable = false;
            }
            handleFinalHead(event);
        }
        break;
    case Http1DecodeEvent::Kind::Body:
        recordBodyProgress();
        if (_bodyMode == BodyMode::Streaming && _callbacks.bodyData) {
            try {
                _callbacks.bodyData(event.data());
            } catch (...) {
                fail(
                    Http1TransactionFailure{
                        Http1TransactionFailure::Kind::Callback,
                        Http1TransactionFailure::Phase::Body,
                        "The HTTP body callback threw an exception."_el},
                    false);
            }
        } else if (_bodyMode == BodyMode::Aggregating && isFailure(_aggregate->writeExact(event.data().span()))) {
            fail(
                Http1TransactionFailure{
                    Http1TransactionFailure::Kind::ResourceLimit,
                    Http1TransactionFailure::Phase::Body,
                    "The incoming HTTP body exceeds its selected aggregate limit."_el,
                    Http1FailureReason::BodyLimitExceeded},
                _role == Role::Server);
        }
        break;
    case Http1DecodeEvent::Kind::Trailers:
        if (_callbacks.trailers) {
            try {
                _callbacks.trailers(event.trailerFields());
            } catch (...) {
                fail(
                    Http1TransactionFailure{
                        Http1TransactionFailure::Kind::Callback,
                        Http1TransactionFailure::Phase::Body,
                        "The HTTP trailer callback threw an exception."_el},
                    false);
            }
        }
        break;
    case Http1DecodeEvent::Kind::Complete:
        if (_informationalInput) {
            _informationalInput = false;
            resetResponseDecoder();
            return;
        }
        _incomingComplete = true;
        _bodyMode = BodyMode::None;
        _incomingContentLength.reset();
        _bufferedInput =
            _role == Role::Client ? _responseDecoder->takeRetainedInput() : _requestDecoder->takeRetainedInput();
        if (_aggregate && _callbacks.aggregatedBody) {
            try {
                _callbacks.aggregatedBody(_aggregate->read(unit::ByteLength::infinite()));
            } catch (...) {
                fail(
                    Http1TransactionFailure{
                        Http1TransactionFailure::Kind::Callback,
                        Http1TransactionFailure::Phase::Body,
                        "The HTTP aggregate-body callback threw an exception."_el},
                    false);
                return;
            }
        }
        if (_callbacks.inputComplete) {
            try {
                _callbacks.inputComplete();
            } catch (...) {
                fail(
                    Http1TransactionFailure{
                        Http1TransactionFailure::Kind::Callback,
                        Http1TransactionFailure::Phase::Body,
                        "The HTTP input-complete callback threw an exception."_el},
                    false);
                return;
            }
        }
        assessCompletion();
        break;
    }
}

void Http1Transaction::handleFinalHead(const Http1DecodeEvent &event) {
    assessPersistence(event);
    _incomingContentLength = event.contentLength();
    _state = State::ReceivingBody;
    if (hasBody(event)) {
        _state = State::AwaitingBodyPolicy;
        _bodyMode = BodyMode::Awaiting;
        _connection->pauseReceiving();
        try {
            if (_role == Role::Server && _callbacks.requestHead) {
                _callbacks.requestHead(event);
            } else if (_role == Role::Client && _callbacks.responseHead) {
                _callbacks.responseHead(event);
            }
        } catch (...) {
            fail(
                Http1TransactionFailure{
                    Http1TransactionFailure::Kind::Callback,
                    Http1TransactionFailure::Phase::Headers,
                    "The HTTP message-head callback threw an exception."_el},
                false);
            return;
        }
        return;
    }
    _bodyMode = BodyMode::None;
    try {
        if (_role == Role::Server && _callbacks.requestHead) {
            _callbacks.requestHead(event);
        } else if (_role == Role::Client && _callbacks.responseHead) {
            _callbacks.responseHead(event);
        }
    } catch (...) {
        fail(
            Http1TransactionFailure{
                Http1TransactionFailure::Kind::Callback,
                Http1TransactionFailure::Phase::Headers,
                "The HTTP message-head callback threw an exception."_el},
            false);
    }
}

auto Http1Transaction::hasBody(const Http1DecodeEvent &event) noexcept -> bool {
    if (event.framing() == Http1BodyFraming::None || event.framing() == Http1BodyFraming::Opaque) {
        return false;
    }
    return !event.contentLength() || !event.contentLength()->isZero();
}

auto Http1Transaction::nextDecodeEvent() -> std::optional<Http1DecodeEvent> {
    return _role == Role::Client ? _responseDecoder->next() : _requestDecoder->next();
}

auto Http1Transaction::feedDecoder(const mem::ConstByteSpan data) -> NetworkSendStatus {
    _incomingStarted = _incomingStarted || !data.empty();
    return _role == Role::Client ? _responseDecoder->feed(data) : _requestDecoder->feed(data);
}

void Http1Transaction::endDecoderInput() {
    if (_role == Role::Client) {
        _responseDecoder->endOfInput();
    } else {
        _requestDecoder->endOfInput();
    }
}

void Http1Transaction::resetResponseDecoder() {
    _responseDecoder->reset();
    _responseDecoder->setRequestMethod(_requestMethod);
}

auto Http1Transaction::finalEncoderQueuedLength() const noexcept -> unit::ByteLength {
    if (_role == Role::Client && _requestEncoder) {
        return _requestEncoder->queuedOutputLength();
    }
    if (_role == Role::Server && _responseEncoder) {
        return _responseEncoder->queuedOutputLength();
    }
    return {};
}

auto Http1Transaction::isFinalEncoderComplete() const noexcept -> bool {
    if (_outgoingTerminated) {
        return true;
    }
    if (_role == Role::Client && _requestEncoder) {
        return _requestEncoder->isComplete();
    }
    if (_role == Role::Server && _responseEncoder) {
        return _responseEncoder->isComplete();
    }
    return false;
}

auto Http1Transaction::takeEncoderOutput() -> mem::ByteBlock {
    if (_informationalEncoder) {
        auto result = _informationalEncoder->takeOutput();
        if (result.isEmpty() && _informationalEncoder->isComplete()) {
            _informationalEncoder.reset();
        }
        return result;
    }
    if (_role == Role::Client && _requestEncoder) {
        return _requestEncoder->takeOutput();
    }
    if (_role == Role::Server && _responseEncoder) {
        return _responseEncoder->takeOutput();
    }
    return {};
}

void Http1Transaction::pumpOutput() {
    if (_finalized || _state == State::Cancelled) {
        return;
    }
    for (;;) {
        if (_pendingOutput) {
            const auto result = _connection->send(*_pendingOutput);
            if (result.wouldBlock()) {
                return;
            }
            if (result.isClosed()) {
                fail(
                    Http1TransactionFailure{
                        Http1TransactionFailure::Kind::Transport,
                        Http1TransactionFailure::Phase::Transport,
                        "The connection closed while accepting HTTP output."_el},
                    false);
                return;
            }
            _pendingOutput.reset();
        }
        auto output = takeEncoderOutput();
        if (output.isEmpty()) {
            return;
        }
        const auto result = _connection->send(output);
        if (result.wouldBlock()) {
            _pendingOutput = std::move(output);
            return;
        }
        if (result.isClosed()) {
            fail(
                Http1TransactionFailure{
                    Http1TransactionFailure::Kind::Transport,
                    Http1TransactionFailure::Phase::Transport,
                    "The connection closed while accepting HTTP output."_el},
                false);
            return;
        }
    }
}

void Http1Transaction::updateOutgoingDrained() {
    _outgoingDrained = _outgoingStarted && isFinalEncoderComplete() && !_pendingOutput &&
        finalEncoderQueuedLength().isZero() && !_informationalEncoder;
    if (_failureEmitted && _role == Role::Server && _outgoingDrained && _state != State::Closing) {
        startClosing();
    }
}

}
