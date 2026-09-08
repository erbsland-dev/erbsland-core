// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpServerConnection.hpp"

#include "HttpRequestTarget.hpp"
#include "HttpServer.hpp"
#include "HttpServerRequest.hpp"
#include "HttpServerSession.hpp"

#include "../codec/Http1BodyFraming.hpp"
#include "../codec/Http1DecodeEvent.hpp"
#include "../codec/Http1Transaction.hpp"
#include "../codec/Http1TransactionFailure.hpp"
#include "../codec/Http1TransactionOptions.hpp"
#include "../HttpGrammar.hpp"

#include "../../../../err/Exception.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/AsciiCategory.hpp"
#include "../../../../text/EncodingMode.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringBomMode.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringDecoder.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../../text/StringEncoder.hpp"
#include "../../../../text/StringEncoding.hpp"
#include "../../../HostEndpoint.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../http/HttpResponseHead.hpp"
#include "../../../http_server/HttpServerSessionContext.hpp"
#include "../../../http_server/HttpServerSessionManager.hpp"
#include "../../../http_server/HttpTlsConnectionInfo.hpp"
#include "../../../Network.hpp"
#include "../../../source/ConnectionEventEditor.hpp"
#include "../../../source/NetworkErrorReason.hpp"
#include "../../../tcp/TcpConnection.hpp"
#include "../../../tcp/TcpConnectionRequest.hpp"
#include "../../../tls/TlsServerConnection.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpServerConnection::HttpServerConnection(std::shared_ptr<HttpServer> server, TcpConnectionRequestPtr request) :
    _server{std::move(server)}, _acceptRequest{std::move(request)} {
}

void HttpServerConnection::start() {
    const auto server = _server.lock();
    if (server == nullptr || _acceptRequest == nullptr) {
        return;
    }
    if (server->_tlsAcceptOptions.has_value()) {
        startTls();
    } else {
        startPlain();
    }
}

void HttpServerConnection::serverClosing() {
    _closing = true;
    if (_transaction == nullptr && _connection != nullptr) {
        _connection->close();
    }
}

void HttpServerConnection::abort() noexcept {
    _closing = true;
    if (_staticContent != nullptr) {
        _staticContent->cancel();
    }
    if (_transaction != nullptr) {
        _transaction->cancel();
    } else if (_connection != nullptr) {
        _connection->abort();
    } else if (_acceptRequest != nullptr) {
        _acceptRequest->reject();
    }
}

void HttpServerConnection::startPlain() {
    const auto server = _server.lock();
    if (server == nullptr) {
        return;
    }
    auto connection = server->ownerEvents()->get<Network>().createTcpConnection();
    const auto weakSelf = std::weak_ptr<HttpServerConnection>{shared_from_this()};
    connection->events()
        .onConnected([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleActive();
            }
        })
        .onError([weakSelf](const NetworkErrorContext &context) -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleSetupError(context);
            }
        })
        .onFinal([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleSetupFinal();
            }
        });
    _connection = connection;
    connection->accept(std::move(_acceptRequest), server->_tcpAcceptOptions);
}

void HttpServerConnection::startTls() {
    const auto server = _server.lock();
    if (server == nullptr) {
        return;
    }
    auto connection = server->ownerEvents()->get<Network>().createTlsServerConnection();
    const auto weakSelf = std::weak_ptr<HttpServerConnection>{shared_from_this()};
    connection->events()
        .onHandshakeCompleted([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleActive();
            }
        })
        .onError([weakSelf](const NetworkErrorContext &context) -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleSetupError(context);
            }
        })
        .onFinal([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleSetupFinal();
            }
        });
    _connection = connection;
    _secure = true;
    connection->accept(std::move(_acceptRequest), *server->_tlsAcceptOptions);
}

void HttpServerConnection::handleActive() {
    const auto server = _server.lock();
    if (server == nullptr) {
        return;
    }
    if (_closing) {
        _connection->close();
        return;
    }
    if (_secure) {
        const auto tlsConnection = std::dynamic_pointer_cast<network::TlsServerConnection>(_connection);
        if (tlsConnection == nullptr || tlsConnection->negotiatedAlpn() != "http/1.1"_el) {
            _connection->abort();
            return;
        }
    }
    _connectionInfo = createConnectionInfo();
    server->notifyConnectionActive(*_connectionInfo);
    startTransaction();
}

void HttpServerConnection::startTransaction() {
    const auto server = _server.lock();
    if (server == nullptr || _connection == nullptr || _connection->state() != ConnectionState::Active) {
        return;
    }
    ++_requestCount;
    _reusable = false;
    _handler = {};
    auto transaction = Http1Transaction::createServer(_connection, transactionOptions());
    const auto weakSelf = std::weak_ptr<HttpServerConnection>{shared_from_this()};
    transaction->callbacks().requestHead = [weakSelf](const Http1DecodeEvent &event) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleRequestHead(event);
        }
    };
    transaction->callbacks().bodyData = [weakSelf](mem::ByteBlock data) -> void {
        if (const auto self = weakSelf.lock(); self != nullptr && self->_request != nullptr) {
            self->_request->deliverBody(std::move(data));
        }
    };
    transaction->callbacks().aggregatedBody = [weakSelf](mem::ByteBlock data) -> void {
        if (const auto self = weakSelf.lock(); self != nullptr && self->_request != nullptr) {
            if (self->_handler.kind() == HttpRouteHandler::Kind::Head) {
                self->_request->deliverAggregatedBody(std::move(data));
            } else {
                self->handleAggregatedBody(std::move(data));
            }
        }
    };
    transaction->callbacks().trailers = [weakSelf](const HttpHeaders &trailers) -> void {
        if (const auto self = weakSelf.lock(); self != nullptr && self->_request != nullptr) {
            self->_request->deliverTrailers(trailers);
        }
    };
    transaction->callbacks().inputComplete = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr && self->_request != nullptr) {
            self->_request->deliverBodyCompleted();
        }
    };
    transaction->callbacks().writable = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr && self->_request != nullptr) {
            self->_request->handleWritable();
            if (self->_staticContent != nullptr) {
                self->_staticContent->handleWritable();
            }
        }
    };
    transaction->callbacks().failure = [weakSelf](const Http1TransactionFailure &failure) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleFailure(failure);
        }
    };
    transaction->callbacks().complete = [weakSelf](const bool reusable) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleComplete(reusable);
        }
    };
    transaction->callbacks().final = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleFinal();
        }
    };
    _transaction = transaction;
    transaction->start();
}

void HttpServerConnection::handleRequestHead(const Http1DecodeEvent &event) {
    const auto server = _server.lock();
    if (server == nullptr) {
        return;
    }
    auto request = std::shared_ptr<HttpServerRequest>{};
    try {
        auto target = HttpRequestTarget{event.request().target()};
        request = std::make_shared<HttpServerRequest>(
            server->ownerEvents(),
            event.request(),
            std::move(target),
            *_connectionInfo,
            _connection,
            HttpRoutes::Parameters{},
            HttpHeaders{},
            server->_options,
            hasBody(event),
            event.request().method() == HttpMethod{HttpMethodType::Head});
        request->attach(_transaction, {});
        _request = request;
    } catch (const err::Exception &error) {
        auto context = NetworkErrorContext{"Invalid HTTP request target"_el, error.reason()};
        context.setReason(NetworkErrorReason::HttpProtocolFailure);
        if (_connection != nullptr) {
            if (const auto local = _connection->localEndpoint(); local.has_value()) {
                context.setLocalEndpoint(*local);
            }
            if (const auto remote = _connection->remoteEndpoint(); remote.has_value()) {
                context.setRemoteEndpoint(HostEndpoint{remote->address(), remote->port(), remote->scopeId()});
            }
        }
        const auto info = _connectionInfo.has_value() ? *_connectionInfo : createConnectionInfo();
        server->notifyConnectionError(info, context);
        sendFrameworkError(HttpStatus::BadRequest);
        return;
    } catch (...) {
        sendFrameworkError(HttpStatus::InternalServerError);
        return;
    }
    try {
        dispatch(request, event);
    } catch (...) {
        if (!request->isResponseStarted()) {
            request->sendError(HttpStatus::InternalServerError, {});
        } else {
            _transaction->cancel();
        }
    }
}

void HttpServerConnection::handleFailure(const Http1TransactionFailure &failure) {
    const auto server = _server.lock();
    if (server != nullptr) {
        auto context =
            failure.transportContext().value_or(NetworkErrorContext{"HTTP connection error"_el, failure.description()});
        if (!failure.transportContext().has_value()) {
            context.setReason(
                failure.kind() == Http1TransactionFailure::Kind::Timeout ? NetworkErrorReason::Timeout
                                                                         : NetworkErrorReason::HttpProtocolFailure);
            if (_connection != nullptr) {
                if (const auto local = _connection->localEndpoint(); local.has_value()) {
                    context.setLocalEndpoint(*local);
                }
                if (const auto remote = _connection->remoteEndpoint(); remote.has_value()) {
                    context.setRemoteEndpoint(HostEndpoint{remote->address(), remote->port(), remote->scopeId()});
                }
            }
        }
        if (_request != nullptr) {
            _request->handleError(context);
        } else {
            const auto info = _connectionInfo.has_value() ? *_connectionInfo : createConnectionInfo();
            server->notifyConnectionError(info, context);
        }
    }
    if (_request != nullptr && _request->isResponseStarted()) {
        return;
    }
    sendFrameworkError(errorStatus(failure));
}

void HttpServerConnection::handleComplete(const bool reusable) {
    _reusable = reusable;
}

void HttpServerConnection::handleFinal() {
    if (_staticContent != nullptr) {
        _staticContent->cancel();
    }
    _staticContent.reset();
    if (_request != nullptr) {
        _request->finalize();
    }
    _request.reset();
    _handler = {};
    _transaction.reset();
    const auto server = _server.lock();
    if (server == nullptr) {
        return;
    }
    const auto maximum = server->_options.maximumRequestsPerConnection().toSizeT();
    if (_reusable && !_closing && server->_state == NetworkSourceState::Active && _requestCount < maximum) {
        const auto weakSelf = std::weak_ptr<HttpServerConnection>{shared_from_this()};
        server->ownerEvents()->invoke([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->startTransaction();
            }
        });
        return;
    }
    if (_reusable && _connection->state() == ConnectionState::Active) {
        const auto weakSelf = std::weak_ptr<HttpServerConnection>{shared_from_this()};
        _connection->events().onFinal([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->handleSetupFinal();
            }
        });
        _connection->close();
        return;
    }
    handleSetupFinal();
}

void HttpServerConnection::handleSetupError(const NetworkErrorContext &context) {
    // Browsers can abandon speculative connections before sending an HTTP request. At this point no authenticated
    // application data exists whose truncation could be security-relevant, so treat peer disconnects as normal.
    const auto normalPeerDisconnect = context.reason() == NetworkErrorReason::TlsTruncation ||
        context.reason() == NetworkErrorReason::ConnectionReset;
    if (normalPeerDisconnect) {
        return;
    }
    if (const auto server = _server.lock(); server != nullptr) {
        const auto info = _connectionInfo.has_value() ? *_connectionInfo : createConnectionInfo();
        server->notifyConnectionError(info, context);
    }
}

void HttpServerConnection::handleSetupFinal() {
    if (_removed) {
        return;
    }
    _removed = true;
    if (const auto server = _server.lock()) {
        if (_connection != nullptr) {
            const auto info = _connectionInfo.has_value() ? *_connectionInfo : createConnectionInfo();
            server->notifyConnectionFinal(info);
        }
        server->removeConnection(shared_from_this());
    }
}

auto HttpServerConnection::createConnectionInfo() const -> HttpConnectionInfo {
    auto localEndpoint = std::optional<IpEndpoint>{};
    auto remoteEndpoint = std::optional<IpEndpoint>{};
    auto tlsInfo = std::optional<HttpTlsConnectionInfo>{};
    if (_connection != nullptr) {
        localEndpoint = _connection->localEndpoint();
        remoteEndpoint = _connection->remoteEndpoint();
        if (const auto tls = std::dynamic_pointer_cast<network::TlsServerConnection>(_connection); tls != nullptr) {
            tlsInfo.emplace(
                tls->requestedConfigurationLabel(),
                tls->matchedConfigurationLabel(),
                tls->serverName(),
                tls->offeredAlpn(),
                tls->negotiatedAlpn(),
                tls->cipherSuite(),
                tls->signatureScheme());
        }
    }
    return HttpConnectionInfo{std::move(localEndpoint), std::move(remoteEndpoint), std::move(tlsInfo)};
}

}
