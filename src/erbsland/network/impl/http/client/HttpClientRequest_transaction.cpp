// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientRequest.hpp"

#include "HttpClientRequestEventEditor.hpp"
#include "HttpClientResponse.hpp"
#include "HttpClientSession.hpp"

#include "../codec/Http1CodecLimits.hpp"
#include "../codec/Http1DecodeEvent.hpp"
#include "../codec/Http1Transaction.hpp"
#include "../codec/Http1TransactionFailure.hpp"
#include "../codec/Http1TransactionOptions.hpp"
#include "../cookie/PublicSuffixList.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../http/HttpRequestHead.hpp"
#include "../../../Network.hpp"
#include "../../../source/Connection.hpp"
#include "../../../source/NetworkErrorContext.hpp"
#include "../../../source/NetworkErrorPhase.hpp"
#include "../../../source/NetworkErrorReason.hpp"
#include "../../../tcp/TcpConnection.hpp"
#include "../../../tcp/TcpConnectionEventEditor.hpp"
#include "../../../tcp/TcpConnectOptions.hpp"
#include "../../../tls/TlsClientConnection.hpp"
#include "../../../tls/TlsClientConnectionEventEditor.hpp"
#include "../../../tls/TlsClientConnectOptions.hpp"
#include "../../ConnectionProtocolAccess.hpp"
#include "../../url/UrlData.hpp"
#include "../../url/UrlWriter.hpp"

#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>

namespace erbsland::network::impl {
using namespace text::literals;

void HttpClientRequest::startTransaction() {
    _transactionFinalized = false;
    auto codecLimits = Http1CodecLimits{};
    codecLimits.setMaximumStartLineLength(_options.maximumStartLineLength())
        .setHeaderLimits(_options.headerLimits())
        .setTrailerLimits(_options.headerLimits())
        .setMaximumBodyLength(_options.maximumBodyLength())
        .setMaximumInputLength(_options.maximumQueueLength())
        .setMaximumOutputLength(_options.maximumQueueLength());
    auto transactionOptions = Http1TransactionOptions{};
    transactionOptions.setCodecLimits(codecLimits)
        .setHeaderTimeout(_options.headerTimeout())
        .setBodyIdleTimeout(_options.bodyIdleTimeout())
        .setTotalTimeout(_options.overallTimeout())
        .setCloseTimeout(_options.closeTimeout());
    _transaction = Http1Transaction::createClient(
        _connection,
        HttpRequestHead{_hopMethod, requestTarget(), HttpVersion::Http11, buildHeaders()},
        transactionOptions);
    const auto weakSelf =
        std::weak_ptr<HttpClientRequest>{std::static_pointer_cast<HttpClientRequest>(shared_from_this())};
    _transaction->callbacks().responseHead = [weakSelf](const Http1DecodeEvent &event) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleResponseHead(event);
        }
    };
    _transaction->callbacks().informationalHead = [weakSelf](const Http1DecodeEvent &event) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleInformational(event);
        }
    };
    _transaction->callbacks().bodyData = [weakSelf](mem::ByteBlock data) -> void {
        if (const auto self = weakSelf.lock(); self && self->_response) {
            self->_response->handleBodyData(std::move(data));
        }
    };
    _transaction->callbacks().aggregatedBody = [weakSelf](mem::ByteBlock data) -> void {
        if (const auto self = weakSelf.lock(); self && self->_response) {
            self->_response->handleAggregatedBody(std::move(data));
        }
    };
    _transaction->callbacks().trailers = [weakSelf](const HttpHeaders &trailers) -> void {
        if (const auto self = weakSelf.lock(); self && self->_response) {
            self->_response->handleTrailers(trailers);
        }
    };
    _transaction->callbacks().inputComplete = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock(); self && self->_response) {
            self->_response->handleBodyCompleted();
        }
    };
    _transaction->callbacks().writable = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock()) {
            if (self->_bodyMode == BodyMode::Fixed && !self->_uploadFinished) {
                self->pumpPreparedBody();
            } else {
                self->_uploadWritablePending = false;
                if (self->_onWritable) {
                    self->_onWritable();
                }
            }
        }
    };
    _transaction->callbacks().failure = [weakSelf](const Http1TransactionFailure &failure) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleTransactionFailure(failure);
        }
    };
    _transaction->callbacks().final = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleTransactionFinal();
        }
    };
    _transaction->start();
    pumpPreparedBody();
}

void HttpClientRequest::pumpPreparedBody() {
    if (_transaction == nullptr || _uploadFinished) {
        return;
    }
    if (_bodyMode == BodyMode::Streaming) {
        _uploadWritablePending = false;
        if (_onWritable) {
            _onWritable();
        }
        return;
    }
    constexpr auto cChunkLength = unit::ByteLength{16U * 1024U};
    while (_bodyMode == BodyMode::Fixed && _fixedBodyOffset.isWithin(_fixedBody.length())) {
        const auto remaining = _fixedBody.length() - unit::ByteLength{_fixedBodyOffset.toRawValue()};
        const auto length = std::min(remaining, cChunkLength);
        const auto block = _fixedBody.slice(_fixedBodyOffset, length);
        const auto result = _transaction->sendBody(block);
        if (result.wouldBlock()) {
            return;
        }
        if (result.isClosed()) {
            return;
        }
        _fixedBodyOffset += length;
    }
    const auto result = _transaction->finishBody();
    _uploadFinished = result.isAccepted();
}

void HttpClientRequest::handleResponseHead(const Http1DecodeEvent &event) {
    _responseObserved = true;
    const auto hasBody = event.framing() != Http1BodyFraming::None && event.framing() != Http1BodyFraming::Opaque &&
        (!event.contentLength() || !event.contentLength()->isZero());
    if (const auto session = _session.lock(); session && _options.automaticCookiesEnabled()) {
        session->_cookieJar.storeResponseCookies(_effectiveUrl, event.response().headers());
    }
    if (handleRedirect(event, hasBody)) {
        return;
    }
    _response = std::make_shared<HttpClientResponse>(
        std::static_pointer_cast<HttpClientRequest>(shared_from_this()),
        event.response(),
        _transaction,
        hasBody,
        _effectiveUrl,
        _redirectCount,
        event.contentLength());
    if (_responseHandler.kind() == HttpClientResponseHandler::Kind::Head) {
        if (_responseHandler.hasCallback()) {
            std::get<HttpClientResponseHeadFn>(_responseHandler.callback())(
                std::static_pointer_cast<HttpClientRequest>(shared_from_this()), _response);
        } else if (hasBody) {
            _response->rejectBody();
        }
        return;
    }
    _response->startAutomatic(_responseHandler);
}

void HttpClientRequest::handleInformational(const Http1DecodeEvent &event) {
    _responseObserved = true;
    if (_onInformational) {
        _onInformational(std::static_pointer_cast<HttpClientRequest>(shared_from_this()), event.response());
    }
}

void HttpClientRequest::handleTransactionFailure(const Http1TransactionFailure &failure) {
    if (_cancelled || _failed || _finalEmitted) {
        return;
    }
    const auto staleConnectionFailure =
        failure.transportContext().has_value() || failure.protocolReason() == Http1FailureReason::PrematureEndOfStream;
    if (staleConnectionFailure && _leasedReused && !_staleRetried && !_responseObserved && canRetryStale()) {
        _retryStale = true;
        _staleRetried = true;
        return;
    }
    if (failure.transportContext()) {
        fail(*failure.transportContext());
        return;
    }
    auto context = NetworkErrorContext{"HTTP client request failed"_el, failure.description()};
    context.setRemoteEndpoint(_effectiveUrl.endpoint());
    switch (failure.kind()) {
    case Http1TransactionFailure::Kind::Timeout:
        context.setReason(NetworkErrorReason::Timeout);
        break;
    case Http1TransactionFailure::Kind::ResourceLimit:
        context.setReason(NetworkErrorReason::ResourceLimitExceeded);
        break;
    default:
        context.setReason(NetworkErrorReason::HttpProtocolFailure);
        break;
    }
    switch (failure.phase()) {
    case Http1TransactionFailure::Phase::Headers:
        context.setPhase(NetworkErrorPhase::HttpResponseHeaders);
        break;
    case Http1TransactionFailure::Phase::Body:
        context.setPhase(NetworkErrorPhase::HttpResponseBody);
        break;
    case Http1TransactionFailure::Phase::Closing:
        context.setPhase(NetworkErrorPhase::Closing);
        break;
    default:
        context.setPhase(NetworkErrorPhase::HttpRequest);
        break;
    }
    fail(std::move(context));
}

void HttpClientRequest::handleTransactionFinal() {
    _transactionFinalized = true;
    if (_response != nullptr && !_response->isFinal()) {
        return;
    }
    completeTransactionFinal();
}

void HttpClientRequest::handleResponseFinal() {
    if (_transactionFinalized && !_finalEmitted) {
        completeTransactionFinal();
    }
}

void HttpClientRequest::completeTransactionFinal() {
    if (_retryStale && !_failed) {
        _transaction.reset();
        _connection.reset();
        _response.reset();
        _fixedBodyOffset = {};
        _uploadFinished = false;
        _leasedReused = false;
        _responseObserved = false;
        _retryStale = false;
        _transactionFinalized = false;
        _state = NetworkSourceState::Starting;
        const auto self = std::static_pointer_cast<HttpClientRequest>(shared_from_this());
        ownerEvents()->invoke([self]() -> void { self->start(); });
        return;
    }
    auto returned = false;
    if (!_failed && _transaction != nullptr && _transaction->isConnectionReusable() && _connection != nullptr &&
        _connection->state() == ConnectionState::Active &&
        ConnectionProtocolAccess::takeRetainedInput(*_connection).isEmpty()) {
        _completedTransactions += unit::ItemCount::one();
        if (const auto session = _session.lock(); session && session->_state != NetworkSourceState::Closed &&
            session->_state != NetworkSourceState::Closing &&
            session->_configurationGeneration == _configurationGeneration) {
            session->_connectionManager->returnConnection(
                _connectionUrl, _configurationGeneration, _connection, _completedTransactions, _options);
            returned = true;
        }
    }
    if (_followingRedirect && !_failed) {
        if (!returned && _connection != nullptr && _connection->state() == ConnectionState::Active) {
            _connection->close();
        }
        startRedirectHop();
        return;
    }
    if (!returned && _connection != nullptr && _connection->state() == ConnectionState::Active) {
        _connection->close();
    }
    finishFinal();
}

}
