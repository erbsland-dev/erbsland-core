// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpServer.hpp"

#include "HttpServerConnection.hpp"
#include "HttpServerEventEditor.hpp"
#include "HttpServerSession.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../mem/ByteBlock.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringBomMode.hpp"
#include "../../../../text/StringEncoder.hpp"
#include "../../../../text/StringEncoding.hpp"
#include "../../../http_server/HttpServerSessionManager.hpp"
#include "../../../http_server/HttpStaticContentHandler.hpp"
#include "../../../Network.hpp"
#include "../../../source/NetworkErrorContext.hpp"
#include "../../../source/NetworkErrorReason.hpp"
#include "../../../tcp/TcpConnectionRequest.hpp"
#include "../../../tcp/TcpListener.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

HttpServer::HttpServer(event::EventsPtr ownerEvents) : network::HttpServer{std::move(ownerEvents)} {
}

HttpServer::~HttpServer() {
    if (_listener != nullptr) {
        _listener->abort();
    }
    for (const auto &connection : _connections) {
        connection->abort();
    }
}

void HttpServer::setOptions(HttpServerOptions options) {
    verifyCurrentOwnerEvents();
    verifyInactive();
    _options = std::move(options);
}

void HttpServer::setListenerOptions(TcpListenerOptions options) {
    verifyCurrentOwnerEvents();
    verifyInactive();
    _listenerOptions = std::move(options);
}

void HttpServer::setTcpAcceptOptions(TcpAcceptOptions options) {
    verifyCurrentOwnerEvents();
    verifyInactive();
    _tcpAcceptOptions = std::move(options);
}

void HttpServer::enableTls() {
    enableTls(HttpServerTlsOptions{});
}

void HttpServer::enableTls(HttpServerTlsOptions options) {
    verifyCurrentOwnerEvents();
    verifyInactive();
    _tlsOptions = std::move(options);
}

void HttpServer::setSessionManager(HttpServerSessionManagerPtr manager) {
    verifyCurrentOwnerEvents();
    verifyInactive();
    _sessionManager = std::move(manager);
}

void HttpServer::addStaticContentHandler(HttpStaticContentHandlerPtr handler) {
    verifyCurrentOwnerEvents();
    verifyInactive();
    if (handler == nullptr) {
        throw err::ParameterError{"An HTTP server requires a valid static-content handler."_el, "handler"_el};
    }
    _staticContentHandlers.emplace_back(std::move(handler));
}

auto HttpServer::localEndpoint() const -> std::optional<IpEndpoint> {
    return _listener != nullptr ? _listener->localEndpoint() : std::nullopt;
}

auto HttpServer::state() const noexcept -> NetworkSourceState {
    return _state;
}

void HttpServer::start(IpEndpoint localEndpoint) {
    verifyCurrentOwnerEvents();
    verifyInactive();
    validateOptions();
    if (_tlsOptions.has_value()) {
        _tlsAcceptOptions = createTlsAcceptOptions();
    }
    auto staticContent = _staticContentHandlers;
    auto staticContentUse = HttpStaticContentUse::create(staticContent);
    std::ranges::stable_sort(staticContent, [](const auto &left, const auto &right) -> bool {
        return left->priority() > right->priority();
    });
    _staticContent = std::move(staticContent);
    _staticContentUse = std::move(staticContentUse);
    try {
        _state = NetworkSourceState::Starting;
        _listener = ownerEvents()->get<Network>().createTcpListener();
        const auto weakSelf = std::weak_ptr<HttpServer>{std::static_pointer_cast<HttpServer>(shared_from_this())};
        _listener->events()
            .onListening([weakSelf]() -> void {
                if (const auto self = weakSelf.lock()) {
                    self->_state = NetworkSourceState::Active;
                    if (self->_onListening) {
                        self->_onListening();
                    }
                }
            })
            .onConnection([weakSelf](TcpConnectionRequestPtr request) -> void {
                if (const auto self = weakSelf.lock()) {
                    self->handleConnection(std::move(request));
                } else if (request != nullptr) {
                    request->reject();
                }
            })
            .onError([weakSelf](const NetworkErrorContext &context) -> void {
                if (const auto self = weakSelf.lock()) {
                    self->handleListenerError(context);
                }
            })
            .onFinal([weakSelf]() -> void {
                if (const auto self = weakSelf.lock()) {
                    self->_listenerFinal = true;
                    self->assessClosed();
                }
            });
        _listener->start(std::move(localEndpoint), _listenerOptions);
    } catch (...) {
        _listener.reset();
        _staticContent.clear();
        _staticContentUse.reset();
        _state = NetworkSourceState::Inactive;
        throw;
    }
}

void HttpServer::pauseAccepting() {
    verifyCurrentOwnerEvents();
    if (_listener != nullptr) {
        _listener->pauseAccepting();
    }
}

void HttpServer::resumeAccepting() {
    verifyCurrentOwnerEvents();
    if (_listener != nullptr && _state == NetworkSourceState::Active) {
        _listener->resumeAccepting();
    }
}

void HttpServer::close() {
    verifyCurrentOwnerEvents();
    if (_state == NetworkSourceState::Closed || _state == NetworkSourceState::Failed ||
        _state == NetworkSourceState::Closing) {
        return;
    }
    if (_state == NetworkSourceState::Inactive) {
        _listenerFinal = true;
    }
    _state = NetworkSourceState::Closing;
    if (_listener != nullptr) {
        _listener->close();
    }
    for (const auto &connection : _connections) {
        connection->serverClosing();
    }
    assessClosed();
}

void HttpServer::abort() noexcept {
    if (_state == NetworkSourceState::Closed || _state == NetworkSourceState::Failed) {
        return;
    }
    _state = NetworkSourceState::Closed;
    if (_listener != nullptr) {
        _listener->abort();
    }
    for (const auto &connection : _connections) {
        connection->abort();
    }
    _connections.clear();
    _staticContentUse.reset();
    if (!_finalEmitted) {
        _finalEmitted = true;
        try {
            const auto callback = _onFinal;
            ownerEvents()->invoke([callback]() -> void {
                if (callback) {
                    callback();
                }
            });
        } catch (...) {}
    }
}

auto HttpServer::events() -> network::HttpServerEventEditor & {
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<HttpServerEventEditor>(shared_from_this(), std::move(target), *this);
    }
    return *_eventEditor;
}

void HttpServer::handleConnection(TcpConnectionRequestPtr request) {
    if (_state != NetworkSourceState::Active || request == nullptr) {
        if (request != nullptr) {
            request->reject();
        }
        return;
    }
    auto connection = std::make_shared<HttpServerConnection>(
        std::static_pointer_cast<HttpServer>(shared_from_this()), std::move(request));
    _connections.emplace_back(connection);
    connection->start();
}

void HttpServer::handleListenerError(const NetworkErrorContext &context) {
    if (_state == NetworkSourceState::Closed || _state == NetworkSourceState::Failed) {
        return;
    }
    _state = NetworkSourceState::Failed;
    if (_onError) {
        _onError(context);
    }
    for (const auto &connection : _connections) {
        connection->abort();
    }
    _connections.clear();
    _staticContentUse.reset();
    if (!_finalEmitted) {
        _finalEmitted = true;
        if (_onFinal) {
            _onFinal();
        }
    }
}

void HttpServer::removeConnection(const std::shared_ptr<HttpServerConnection> &connection) {
    const auto iterator = std::ranges::find(_connections, connection);
    if (iterator != _connections.end()) {
        _connections.erase(iterator);
    }
    assessClosed();
}

auto HttpServer::createSession(std::optional<text::String> identifier, HttpSessionDataPtr data)
    -> network::HttpServerSessionPtr {
    const auto weakSelf = std::weak_ptr<HttpServer>{std::static_pointer_cast<HttpServer>(shared_from_this())};
    auto session = std::make_shared<impl::HttpServerSession>(
        ownerEvents(),
        std::move(identifier),
        std::move(data),
        [weakSelf](const network::HttpServerSessionPtr &invalidated) -> void {
            if (const auto self = weakSelf.lock()) {
                self->sessionInvalidated(invalidated);
            }
        },
        [weakSelf](const network::HttpServerSessionPtr &renewed) -> bool {
            if (const auto self = weakSelf.lock(); self != nullptr && self->_sessionManager != nullptr) {
                const auto renewal = self->_sessionManager->renewSession(renewed);
                if (renewal.isValid()) {
                    if (const auto concrete = std::dynamic_pointer_cast<impl::HttpServerSession>(renewed)) {
                        concrete->setRenewal(renewal.identifier(), renewal.responseFields());
                        return true;
                    }
                }
            }
            return false;
        });
    if (_onNewSession) {
        _onNewSession(session);
    }
    return session;
}

void HttpServer::notifyConnectionActive(const HttpConnectionInfo &info) const {
    if (_onConnectionActive) {
        _onConnectionActive(info);
    }
}

void HttpServer::notifyConnectionFinal(const HttpConnectionInfo &info) const {
    if (_onConnectionFinal) {
        _onConnectionFinal(info);
    }
}

void HttpServer::notifyConnectionError(const HttpConnectionInfo &info, const NetworkErrorContext &error) const {
    if (_onConnectionError) {
        _onConnectionError(info, error);
    }
}

void HttpServer::sessionInvalidated(const network::HttpServerSessionPtr &session) {
    if (_sessionManager != nullptr) {
        auto fields = _sessionManager->sessionInvalidated(session);
        if (const auto concrete = std::dynamic_pointer_cast<impl::HttpServerSession>(session)) {
            concrete->setInvalidationResponseFields(std::move(fields));
        }
    }
}

void HttpServer::assessClosed() {
    if (_state != NetworkSourceState::Closing || !_listenerFinal || !_connections.empty() || _finalEmitted) {
        return;
    }
    _state = NetworkSourceState::Closed;
    _finalEmitted = true;
    _staticContentUse.reset();
    if (_onClosed) {
        _onClosed();
    }
    if (_onFinal) {
        _onFinal();
    }
}

void HttpServer::verifyInactive() const {
    if (_state != NetworkSourceState::Inactive) {
        throw err::LogicError{"An HTTP server can only be configured while inactive."_el};
    }
}

void HttpServer::validateOptions() const {
    if (_options.maximumStartLineLength().isZero() || _options.maximumStartLineLength().isInfinite() ||
        _options.maximumBodyLength().isZero() || _options.maximumBodyLength().isInfinite() ||
        _options.maximumQueueLength().isZero() || _options.maximumQueueLength().isInfinite() ||
        _options.maximumFixedResponseLength().isInfinite() ||
        _options.maximumFixedResponseLength() > _options.maximumQueueLength() / 2U ||
        _options.maximumRequestsPerConnection().isZero() || _options.maximumRequestsPerConnection().isInfinite() ||
        _options.maximumStaticContentResponses().isZero() || _options.maximumStaticContentResponses().isInfinite() ||
        _options.maximumStaticContentOperations().isZero() || _options.maximumStaticContentOperations().isInfinite() ||
        _options.maximumStaticContentQueueLength().isZero() ||
        _options.maximumStaticContentQueueLength().isInfinite() ||
        _options.maximumRetainedStaticContentMemoryLength().isZero() ||
        _options.maximumRetainedStaticContentMemoryLength().isInfinite() ||
        _options.staticContentChunkLength().isZero() || _options.staticContentChunkLength().isInfinite() ||
        _options.staticContentChunkLength() > HttpServerOptions::cDefaultStaticContentChunkLength ||
        _options.staticContentChunkLength() > _options.maximumStaticContentQueueLength() ||
        !_options.headerTimeout().isPositive() || !_options.bodyIdleTimeout().isPositive() ||
        !_options.totalTimeout().isPositive() || !_options.closeTimeout().isPositive()) {
        throw err::ParameterError{"The HTTP server limits and deadlines are invalid."_el, "options"_el};
    }
    if (_tlsOptions.has_value() &&
        (_tlsOptions->maximumConcurrentHandshakes().isZero() ||
            _tlsOptions->maximumConcurrentHandshakes().isInfinite() || !_tlsOptions->handshakeTimeout().isPositive() ||
            _tlsOptions->identityMappings().size() > TlsServerAcceptOptions::cMaximumIdentityMappings.toSizeT())) {
        throw err::ParameterError{"The HTTP server TLS options are invalid."_el, "tlsOptions"_el};
    }
}

auto HttpServer::reserveStaticResponse() noexcept -> bool {
    if (_activeStaticResponses >= _options.maximumStaticContentResponses().toSizeT()) {
        return false;
    }
    ++_activeStaticResponses;
    return true;
}

void HttpServer::releaseStaticResponse() noexcept {
    if (_activeStaticResponses != 0U) {
        --_activeStaticResponses;
    }
}

auto HttpServer::reserveStaticOperation() noexcept -> bool {
    if (_activeStaticOperations >= _options.maximumStaticContentOperations().toSizeT()) {
        return false;
    }
    ++_activeStaticOperations;
    return true;
}

void HttpServer::releaseStaticOperation() noexcept {
    if (_activeStaticOperations != 0U) {
        --_activeStaticOperations;
    }
}

auto HttpServer::reserveStaticQueue(const unit::ByteLength length) noexcept -> bool {
    const auto maximum = _options.maximumStaticContentQueueLength().toRawValue();
    if (length.isInfinite() || length.toRawValue() > maximum - std::min(maximum, _staticQueueLength)) {
        return false;
    }
    _staticQueueLength += length.toRawValue();
    return true;
}

void HttpServer::releaseStaticQueue(const unit::ByteLength length) noexcept {
    _staticQueueLength -= std::min(_staticQueueLength, length.toRawValue());
}

auto HttpServer::reserveStaticMemory(const unit::ByteLength length) noexcept -> bool {
    const auto maximum = _options.maximumRetainedStaticContentMemoryLength().toRawValue();
    if (length.isInfinite() || length.toRawValue() > maximum - std::min(maximum, _staticMemoryLength)) {
        return false;
    }
    _staticMemoryLength += length.toRawValue();
    return true;
}

void HttpServer::releaseStaticMemory(const unit::ByteLength length) noexcept {
    _staticMemoryLength -= std::min(_staticMemoryLength, length.toRawValue());
}

auto HttpServer::createTlsAcceptOptions() const -> TlsServerAcceptOptions {
    const auto &httpTls = *_tlsOptions;
    auto result = TlsServerAcceptOptions{ConnectionQuota::create(httpTls.maximumConcurrentHandshakes())};
    result.setConfigurationLabel(httpTls.configurationLabel())
        .setIdentityMappings(httpTls.identityMappings())
        .setTcpOptions(_tcpAcceptOptions)
        .setAlpnProtocols({"http/1.1"_el})
        .setHandshakeTimeout(httpTls.handshakeTimeout())
        .setCloseTimeout(_options.closeTimeout());
    return result;
}

}
