// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Http1Transaction.hpp"

#include "../HttpGrammar.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/CharSet.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../ConnectionProtocolAccess.hpp"

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

void Http1Transaction::validateStart() const {
    if (_connection->state() != ConnectionState::Active) {
        throw err::LogicError{"An HTTP transaction requires an active connection."_el};
    }
    if (!_options.headerTimeout().isPositive() || !_options.bodyIdleTimeout().isPositive() ||
        !_options.totalTimeout().isPositive() || !_options.closeTimeout().isPositive()) {
        throw err::LogicError{"HTTP transaction deadlines must all be positive."_el};
    }
}

void Http1Transaction::recordBodyProgress() {
    if (_bodyMode == BodyMode::Awaiting || _bodyMode == BodyMode::Rejected || _streamPaused || _failureEmitted) {
        return;
    }
    _bodyDeadline = time::TimePoint::inFuture(_options.bodyIdleTimeout());
    const auto weakSelf = Http1TransactionWeakPtr{shared_from_this()};
    const auto generation = _timerGeneration;
    const auto deadline = _bodyDeadline;
    _connection->ownerEvents()->invokeAfter(_options.bodyIdleTimeout(), [weakSelf, generation, deadline]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleBodyTimeout(generation, deadline);
        }
    });
}

void Http1Transaction::startInitialTimers() {
    const auto weakSelf = Http1TransactionWeakPtr{shared_from_this()};
    const auto generation = _timerGeneration;
    _connection->ownerEvents()->invokeAfter(_options.headerTimeout(), [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleHeaderTimeout(generation);
        }
    });
    _connection->ownerEvents()->invokeAfter(_options.totalTimeout(), [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleTotalTimeout(generation);
        }
    });
}

void Http1Transaction::startClosing() {
    if (_state == State::Closing || _finalized) {
        return;
    }
    _state = State::Closing;
    _reusable = false;
    _connection->close();
    const auto weakSelf = Http1TransactionWeakPtr{shared_from_this()};
    const auto generation = _timerGeneration;
    _connection->ownerEvents()->invokeAfter(_options.closeTimeout(), [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleCloseTimeout(generation);
        }
    });
}

void Http1Transaction::handleHeaderTimeout(const std::uint64_t generation) {
    if (generation != _timerGeneration || _state != State::ReceivingHeaders || _incomingComplete) {
        return;
    }
    fail(
        Http1TransactionFailure{
            Http1TransactionFailure::Kind::Timeout,
            Http1TransactionFailure::Phase::Headers,
            "The HTTP message head timed out."_el},
        _role == Role::Server);
}

void Http1Transaction::handleBodyTimeout(const std::uint64_t generation, const time::TimePoint deadline) {
    if (generation != _timerGeneration || _incomingComplete || _failureEmitted || _state != State::ReceivingBody ||
        _bodyMode == BodyMode::Awaiting || _streamPaused || deadline != _bodyDeadline) {
        return;
    }
    const auto now = time::TimePoint::now();
    if (now < deadline) {
        const auto weakSelf = Http1TransactionWeakPtr{shared_from_this()};
        _connection->ownerEvents()->invokeAfter(now.timeDeltaTo(deadline), [weakSelf, generation, deadline]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleBodyTimeout(generation, deadline);
            }
        });
        return;
    }
    fail(
        Http1TransactionFailure{
            Http1TransactionFailure::Kind::Timeout,
            Http1TransactionFailure::Phase::Body,
            "The HTTP body became idle for too long."_el},
        _role == Role::Server);
}

void Http1Transaction::handleTotalTimeout(const std::uint64_t generation) {
    if (generation != _timerGeneration || _finalized) {
        return;
    }
    fail(
        Http1TransactionFailure{
            Http1TransactionFailure::Kind::Timeout,
            Http1TransactionFailure::Phase::Total,
            "The absolute HTTP transaction deadline expired."_el},
        false);
}

void Http1Transaction::handleCloseTimeout(const std::uint64_t generation) {
    if (generation != _timerGeneration || _state != State::Closing || _finalized) {
        return;
    }
    _connection->abort();
    finishFinal();
}

void Http1Transaction::assessCompletion() {
    updateOutgoingDrained();
    if (_failureEmitted) {
        if (_role == Role::Server && _outgoingDrained) {
            startClosing();
        }
        return;
    }
    if (_incomingComplete && _outgoingDrained) {
        finishSuccess();
    }
}

void Http1Transaction::finishSuccess() {
    if (_completionEmitted || _failureEmitted || _finalized) {
        return;
    }
    _completionEmitted = true;
    _state = State::Completed;
    if (_callbacks.complete) {
        try {
            _callbacks.complete(_reusable);
        } catch (...) {
            _completionEmitted = false;
            fail(
                Http1TransactionFailure{
                    Http1TransactionFailure::Kind::Callback,
                    Http1TransactionFailure::Phase::Transport,
                    "The HTTP completion callback threw an exception."_el},
                false);
            return;
        }
    }
    if (_reusable) {
        ConnectionProtocolAccess::storeRetainedInput(*_connection, std::move(_bufferedInput));
        unbindConnection();
        finishFinal();
    } else {
        startClosing();
    }
}

void Http1Transaction::fail(Http1TransactionFailure failure, const bool serverCanRespond) {
    if (_failureEmitted || _finalized) {
        return;
    }
    _failureEmitted = true;
    _state = State::Failed;
    _reusable = false;
    ConnectionProtocolAccess::storeRetainedInput(*_connection, {});
    _connection->pauseReceiving();
    if (_callbacks.failure) {
        try {
            _callbacks.failure(failure);
        } catch (...) {
            // A failure callback cannot replace or duplicate the original terminal failure.
        }
    }
    if (_role == Role::Server && serverCanRespond) {
        if (_outgoingDrained) {
            startClosing();
        }
        return;
    }
    ++_timerGeneration;
    _connection->abort();
    finishFinal();
}

void Http1Transaction::finishFinal() {
    if (_finalized) {
        return;
    }
    _finalized = true;
    ConnectionProtocolAccess::release(*_connection);
    ++_timerGeneration;
    if (_callbacks.final) {
        try {
            _callbacks.final();
        } catch (...) {
            // Finalization is exactly once even when its observer throws.
        }
    }
}

void Http1Transaction::assessPersistence(const Http1DecodeEvent &event) {
    const auto version =
        event.kind() == Http1DecodeEvent::Kind::RequestHead ? event.request().version() : event.response().version();
    const auto &headers =
        event.kind() == Http1DecodeEvent::Kind::RequestHead ? event.request().headers() : event.response().headers();
    if (version == HttpVersion::Http10 || requestsConnectionClose(headers) ||
        event.framing() == Http1BodyFraming::CloseDelimited || event.framing() == Http1BodyFraming::Opaque) {
        _reusable = false;
    }
}

auto Http1Transaction::requestsConnectionClose(const HttpHeaders &headers) -> bool {
    static const auto cOws = CharSet{U' ', U'\t'};
    const auto values = headers.getAll(HttpFieldType::Connection);
    for (const auto &value : values) {
        auto reader = StringCharReader{value};
        while (!reader.isAtEnd()) {
            reader.advanceWhile(cOws);
            reader.startCapture();
            reader.advanceWhile(http_grammar::tokenCharacters());
            if (http_grammar::equalTokenCI(reader.takeCapture().toString(), "close"_el)) {
                return true;
            }
            reader.advanceWhile(cOws);
            if (!reader.advanceIf(U',')) {
                break;
            }
        }
    }
    return false;
}

}
