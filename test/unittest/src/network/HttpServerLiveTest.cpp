// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/network/http_server/HttpCookieSessionManager.hpp>
#include <erbsland/network/http_server/HttpMediaTypeMapping.hpp>
#include <erbsland/network/http_server/HttpServer.hpp>
#include <erbsland/network/http_server/HttpServerRequest.hpp>
#include <erbsland/network/http_server/HttpServerSession.hpp>
#include <erbsland/network/http_server/HttpStaticContent.hpp>
#include <erbsland/network/http_server/HttpStaticContentHandler.hpp>
#include <erbsland/network/http_server/HttpStaticFileHandler.hpp>
#include <erbsland/network/http_server/HttpStaticResourceHandler.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/source/ConnectionQuota.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>
#include <erbsland/network/tcp/TcpConnection.hpp>
#include <erbsland/network/tls/TlsClientConnection.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/stream/ByteBlockInputStream.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimeUnitTags.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HttpServer HttpServerConnection HttpServerRequest HttpServerSession)
class HttpServerLiveTest final : public el::UnitTest {
private:
    /// Third-party content implementation used to prove public extensibility.
    class DynamicContent final : public HttpStaticContent {
    public:
        DynamicContent() : _data{bytes("custom-content")} {}

        [[nodiscard]] auto length() const noexcept -> el::unit::ByteLength override { return _data.length(); }
        [[nodiscard]] auto open() -> el::stream::ByteInputStreamPtr override {
            if (_opened.exchange(true)) {
                throw el::err::LogicError{"Dynamic test content was opened more than once."_el};
            }
            return std::make_shared<el::stream::ByteBlockInputStream>(_data);
        }

    private:
        el::mem::ByteBlock _data;
        std::atomic_bool _opened{};
    };

    /// Third-party handler unknown to all network internals.
    class DynamicHandler final : public HttpStaticContentHandler {
    public:
        DynamicHandler() : HttpStaticContentHandler{"/custom"_el} {}

        [[nodiscard]] auto hasPath(const el::path::Path &relativePath) const -> bool override {
            return relativePath == el::path::Path{"content.txt"_el};
        }
        [[nodiscard]] auto getContent(const el::path::Path &relativePath) const -> HttpStaticContentPtr override {
            if (!hasPath(relativePath)) {
                throw el::err::LogicError{"Dynamic test content was requested for an unknown path."_el};
            }
            return std::make_shared<DynamicContent>();
        }
    };

    [[nodiscard]] static auto bytes(const std::string_view text) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromSpan(std::span<const char>{text.data(), text.size()});
    }

    [[nodiscard]] static auto readText(const char *path) -> el::text::String {
        return el::text::String{el::unittest::fh::readDataText(path)};
    }

    static void registerTlsConfigurations() {
        auto serverConfiguration = el::cryptology::TlsConfiguration{};
        serverConfiguration.setServerIdentity(
            el::cryptology::TlsServerIdentity{
                el::cryptology::X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/server.pem")),
                el::cryptology::SigningPrivateKey::fromPemOrThrow(
                    readText("data/network/tls-interop/server-key.pem"))});
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            "http"_el, std::move(serverConfiguration));
        auto clientConfiguration = el::cryptology::TlsConfiguration{el::cryptology::X509ServerCertificatePolicy{
            el::cryptology::X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/ca.pem"))}};
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            "tls/client"_el, std::move(clientConfiguration));
    }

    [[nodiscard]] static auto raw(const el::mem::ByteBlock &block) -> std::string {
        auto result = std::string{};
        for (const auto byte : block.span()) {
            result.push_back(byte.toChar());
        }
        return result;
    }

    [[nodiscard]] static auto occurrences(const std::string &text, const std::string_view needle) -> std::size_t {
        auto result = std::size_t{};
        auto offset = std::size_t{};
        while ((offset = text.find(needle, offset)) != std::string::npos) {
            ++result;
            offset += needle.size();
        }
        return result;
    }

    template <typename Predicate>
    void runUntil(const EventLoopPtr &loop, Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(el::time::TimeDelta{el::time::Milliseconds{25}}));
        }
        REQUIRE(predicate());
    }

public:
    void testStaticFileAndApplicationResourceHandlers() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto client = TcpConnectionPtr{};
        auto mediaTypes = HttpMediaTypeMappingPtr{};
        auto customHandler = std::shared_ptr<DynamicHandler>{};
        auto resourceHandler = HttpStaticResourceHandlerPtr{};
        auto fileHandler = HttpStaticFileHandlerPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto responseWire = std::string{};
        auto activeConnections = std::size_t{};
        auto finalConnections = std::size_t{};
        auto sessions = std::size_t{};
        auto requests = std::size_t{};
        auto responses = std::size_t{};
        auto connectionErrors = std::size_t{};
        auto requestErrors = std::size_t{};
        auto responseConnectionInfoAvailable = true;
        auto serverFinal = false;
        auto clientFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            auto options = HttpServerOptions{};
            options.setStaticContentChunkLength(el::unit::ByteLength{8U});
            server->setOptions(options);
            const auto fileRoot = el::path::Path{el::unittest::fh::resolveDataPath("data/network/tls-interop")};
            auto overlay = HttpStaticFileHandler::create(fileRoot, "/res"_el);
            overlay->setPriority(10);
            server->addStaticContentHandler(overlay);
            resourceHandler = HttpStaticResourceHandler::create("test"_el, "/res"_el);
            server->addStaticContentHandler(resourceHandler);
            mediaTypes = HttpMediaTypeMapping::defaultMapping()->copy();
            mediaTypes->setSuffix(".pem"_el, "application/x-pem-file"_el);
            auto files = HttpStaticFileHandler::create(fileRoot, "/files"_el);
            files->setIndexFileNames(el::text::StringList{{"server.pem"_el}}).setMediaTypeMapping(mediaTypes);
            fileHandler = files;
            server->addStaticContentHandler(files);
            customHandler = std::make_shared<DynamicHandler>();
            customHandler->setMediaTypeMapping(mediaTypes);
            server->addStaticContentHandler(customHandler);
            server->events()
                .onConnectionActive([&](const HttpConnectionInfo &info) -> void {
                    ++activeConnections;
                    REQUIRE(info.localEndpoint().has_value());
                    REQUIRE(info.remoteEndpoint().has_value());
                    REQUIRE_FALSE(info.isSecure());
                })
                .onConnectionFinal([&](const HttpConnectionInfo &info) -> void {
                    ++finalConnections;
                    REQUIRE(info.localEndpoint().has_value());
                    REQUIRE(info.remoteEndpoint().has_value());
                })
                .onConnectionError(
                    [&](const HttpConnectionInfo &, const NetworkErrorContext &) -> void { ++connectionErrors; })
                .onNewSession([&](HttpServerSessionPtr session) -> void {
                    ++sessions;
                    session->events().onRequestReceived([&](HttpServerRequestPtr request) -> void {
                        ++requests;
                        REQUIRE(request->connectionInfo().localEndpoint().has_value());
                        REQUIRE(request->connectionInfo().remoteEndpoint().has_value());
                        const auto weakRequest = std::weak_ptr<HttpServerRequest>{request};
                        request->events()
                            .onResponseCommitted([&, weakRequest](const HttpResponseHead &) -> void {
                                ++responses;
                                const auto retainedRequest = weakRequest.lock();
                                responseConnectionInfoAvailable = responseConnectionInfoAvailable &&
                                    retainedRequest != nullptr &&
                                    retainedRequest->connectionInfo().localEndpoint().has_value() &&
                                    retainedRequest->connectionInfo().remoteEndpoint().has_value();
                            })
                            .onError([&](const NetworkErrorContext &) -> void { ++requestErrors; });
                    });
                })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/res/dynamic"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendText("dynamic"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void {
                    REQUIRE_NOTHROW(mediaTypes->setSuffix(".pem"_el, "application/x-released"_el));
                    REQUIRE_NOTHROW(customHandler->setPriority(2));
                    REQUIRE_NOTHROW(resourceHandler->setMaximumContentLength(el::unit::ByteLength{2048U}));
                    REQUIRE_NOTHROW(fileHandler->setIndexFileNames(el::text::StringList{{"index.html"_el}}));
                    serverFinal = true;
                })
                .onListening([&]() -> void {
                    REQUIRE_THROWS_AS(
                        el::err::LogicError, mediaTypes->setSuffix(".pem"_el, "application/x-mutated"_el));
                    REQUIRE_THROWS_AS(el::err::LogicError, customHandler->setUrlPrefix("/changed"_el));
                    REQUIRE_THROWS_AS(
                        el::err::LogicError, resourceHandler->setMaximumContentLength(el::unit::ByteLength{2048U}));
                    REQUIRE_THROWS_AS(
                        el::err::LogicError, fileHandler->setIndexFileNames(el::text::StringList{{"index.html"_el}}));
                    client = loop->get<Network>().createTcpConnection();
                    client->events()
                        .onConnected([&]() -> void {
                            REQUIRE(client
                                    ->send(bytes(
                                        "GET /res/plain.txt HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "HEAD /res/compressed.txt HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "POST /res/plain.txt HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "GET /res/missing.txt HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "GET /res/dynamic HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "GET /files/?x=1 HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "GET /files/server.pem HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "GET /custom/content.txt HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n"))
                                    .isAccepted());
                        })
                        .onData([&](el::mem::ByteBlock data) -> void { responseWire += raw(data); })
                        .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                        .onFinal([&]() -> void {
                            clientFinal = true;
                            server->close();
                        });
                    const auto endpoint = *server->localEndpoint();
                    client->connect(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()});
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (serverFinal && clientFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 405"), 1U);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 404"), 1U);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 500"), 0U);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 200"), 6U);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 308"), 0U);
        REQUIRE(responseWire.find("Content-Type: text/plain") != std::string::npos);
        REQUIRE(responseWire.find("Content-Type: application/x-pem-file") != std::string::npos);
        REQUIRE(responseWire.find("application/x-mutated") == std::string::npos);
        REQUIRE(responseWire.find("Allow: GET, HEAD") != std::string::npos);
        REQUIRE(responseWire.find("hello") != std::string::npos);
        REQUIRE(responseWire.find("dynamic") != std::string::npos);
        REQUIRE(responseWire.find("BEGIN CERTIFICATE") != std::string::npos);
        REQUIRE(responseWire.find("custom-content") != std::string::npos);
        REQUIRE_EQUAL(activeConnections, 1U);
        REQUIRE_EQUAL(finalConnections, 1U);
        REQUIRE_EQUAL(sessions, 1U);
        REQUIRE_EQUAL(requests, 8U);
        REQUIRE_EQUAL(responses, 8U);
        REQUIRE_EQUAL(connectionErrors, 0U);
        REQUIRE_EQUAL(requestErrors, 0U);
        REQUIRE(responseConnectionInfoAvailable);
    }

    void testPlaintextPipeliningRoutingSessionsAndGracefulClose() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto client = TcpConnectionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto responseWire = std::string{};
        auto firstRequest = HttpServerRequestPtr{};
        auto selectedSession = HttpServerSessionPtr{};
        auto newSessionCount = std::size_t{};
        auto requestFinalCount = std::size_t{};
        auto serverClosed = false;
        auto serverFinal = false;
        auto clientFinal = false;
        auto closeStarted = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onNewSession([&](HttpServerSessionPtr session) -> void {
                    ++newSessionCount;
                    selectedSession = session;
                    session->events().onRequest(
                        HttpMethod{HttpMethodType::Get},
                        "/private"_el,
                        [&](HttpServerSessionPtr routeSession,
                            HttpServerRequestPtr request,
                            el::mem::ByteBlock) -> void {
                            REQUIRE_EQUAL(routeSession, selectedSession);
                            REQUIRE_EQUAL(request->session(), selectedSession);
                            request->events().onFinal([&]() -> void { ++requestFinalCount; });
                            request->sendText("session"_el);
                        });
                })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/users/{id}"_el,
                    [&](HttpServerSessionPtr session, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        REQUIRE_EQUAL(session, selectedSession);
                        REQUIRE_EQUAL(request->query(), request->head().target().contains("?"_el) ? "x=1"_el : ""_el);
                        request->events().onFinal([&]() -> void { ++requestFinalCount; });
                        if (firstRequest == nullptr) {
                            firstRequest = request;
                        } else {
                            REQUIRE(firstRequest->isFinal());
                            firstRequest->sendText("stale"_el);
                        }
                        REQUIRE(request->parameter("id"_el).has_value());
                        request->sendText(*request->parameter("id"_el));
                    })
                .onClosed([&]() -> void { serverClosed = true; })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    client = loop->get<Network>().createTcpConnection();
                    client->events()
                        .onConnected([&]() -> void {
                            REQUIRE(client
                                    ->send(bytes(
                                        "GET /users/42?x=1 HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "GET /private HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "PUT /users/42 HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "GET /missing HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "HEAD /users/77 HTTP/1.1\r\nHost: local\r\n\r\n"))
                                    .isAccepted());
                        })
                        .onData([&](el::mem::ByteBlock data) -> void {
                            responseWire += raw(data);
                            if (!closeStarted && occurrences(responseWire, "HTTP/1.1 ") == 5U) {
                                closeStarted = true;
                                client->close();
                                server->close();
                            }
                        })
                        .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                        .onFinal([&]() -> void { clientFinal = true; });
                    const auto endpoint = *server->localEndpoint();
                    client->connect(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()});
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (serverFinal && clientFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE(serverClosed);
        REQUIRE(serverFinal);
        REQUIRE(clientFinal);
        REQUIRE_EQUAL(newSessionCount, 1U);
        REQUIRE_EQUAL(requestFinalCount, 3U);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 200"), 3U);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 405"), 1U);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 404"), 1U);
        REQUIRE(responseWire.find("Allow: GET, HEAD") != std::string::npos);
        REQUIRE(responseWire.find("42") != std::string::npos);
        REQUIRE(responseWire.find("session") != std::string::npos);
    }

    void testBodyPoliciesAndStreamedResponse() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto client = TcpConnectionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto responseWire = std::string{};
        auto streamedInput = std::string{};
        auto inputTrailer = el::text::String{};
        auto serverFinal = false;
        auto clientFinal = false;
        auto closeStarted = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Post},
                    "/aggregate"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock body) -> void {
                        request->sendResponse(
                            HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el}, std::move(body));
                    })
                .onRequestHead(
                    HttpMethod{HttpMethodType::Get},
                    "/stream-response"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request) -> void {
                        auto headers = HttpHeaders{};
                        headers.setField(HttpFieldType::TransferEncoding, "chunked"_el);
                        request->startResponse(HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el, headers});
                        REQUIRE(request->sendBody(bytes("a")).isAccepted());
                        REQUIRE(request->sendBody(bytes("b")).isAccepted());
                        auto trailers = HttpHeaders{};
                        trailers.addField("X-End"_el, "yes"_el);
                        request->finishBody(std::move(trailers));
                    })
                .onRequestHead(
                    HttpMethod{HttpMethodType::Post},
                    "/stream-body"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request) -> void {
                        request->events()
                            .onBodyData([&](el::mem::ByteBlock body) -> void { streamedInput += raw(body); })
                            .onTrailers([&](const HttpHeaders &trailers) -> void {
                                inputTrailer = trailers.getFirst("X-Input"_el);
                            })
                            .onBodyCompleted(
                                [&, request]() -> void { request->sendText(el::text::String{streamedInput}); });
                        request->streamBody();
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    client = loop->get<Network>().createTcpConnection();
                    client->events()
                        .onConnected([&]() -> void {
                            REQUIRE(client
                                    ->send(bytes(
                                        "POST /aggregate HTTP/1.1\r\nHost: local\r\nContent-Length: 5\r\n\r\nhello"
                                        "GET /stream-response HTTP/1.1\r\nHost: local\r\n\r\n"
                                        "POST /stream-body HTTP/1.1\r\nHost: local\r\nTransfer-Encoding: chunked\r\n"
                                        "Trailer: X-Input\r\n\r\n2\r\nab\r\n2\r\ncd\r\n0\r\nX-Input: yes\r\n\r\n"))
                                    .isAccepted());
                        })
                        .onData([&](el::mem::ByteBlock data) -> void {
                            responseWire += raw(data);
                            if (!closeStarted && occurrences(responseWire, "HTTP/1.1 ") == 3U &&
                                responseWire.find("abcd") != std::string::npos) {
                                closeStarted = true;
                                client->close();
                                server->close();
                            }
                        })
                        .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                        .onFinal([&]() -> void { clientFinal = true; });
                    const auto endpoint = *server->localEndpoint();
                    client->connect(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()});
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (serverFinal && clientFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(streamedInput, "abcd");
        REQUIRE_EQUAL(inputTrailer, "yes"_el);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 200"), 3U);
        REQUIRE(responseWire.find("hello") != std::string::npos);
        REQUIRE(responseWire.find("X-End: yes") != std::string::npos);
    }

    void testAutomaticByteTextAndJsonRoutes() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto client = TcpConnectionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto responseWire = std::string{};
        auto serverFinal = false;
        auto clientFinal = false;
        auto closeStarted = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            auto byteOptions = HttpServerRouteOptions{};
            byteOptions.setAcceptedContentTypes({"application/octet-stream"_el});
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Post},
                    "/bytes"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock body) -> void {
                        request->sendResponse(
                            HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el}, std::move(body));
                    },
                    byteOptions)
                .onTextRequest(
                    HttpMethod{HttpMethodType::Post},
                    "/text"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::text::String body) -> void {
                        request->sendText(std::move(body));
                    })
                .onJsonRequest(
                    HttpMethod{HttpMethodType::Post},
                    "/json"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::text::json::JsonValue body) -> void {
                        request->sendJson(body);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    client = loop->get<Network>().createTcpConnection();
                    client->events()
                        .onConnected([&]() -> void {
                            REQUIRE(client
                                    ->send(bytes(
                                        "POST /bytes HTTP/1.1\r\nHost: local\r\nContent-Type: "
                                        "application/octet-stream\r\n"
                                        "Content-Length: 3\r\n\r\nabc"
                                        "POST /text HTTP/1.1\r\nHost: local\r\nContent-Type: text/plain; "
                                        "charset=utf-8\r\n"
                                        "Content-Length: 5\r\n\r\nhello"
                                        "POST /json HTTP/1.1\r\nHost: local\r\n"
                                        "Content-Type: application/merge-patch+json\r\nContent-Length: "
                                        "9\r\n\r\n{\"id\":12}"
                                        "POST /json HTTP/1.1\r\nHost: local\r\nContent-Type: application/json\r\n"
                                        "Content-Length: 1\r\n\r\n{"))
                                    .isAccepted());
                        })
                        .onData([&](el::mem::ByteBlock data) -> void {
                            responseWire += raw(data);
                            if (!closeStarted && occurrences(responseWire, "HTTP/1.1 ") == 4U) {
                                closeStarted = true;
                                client->close();
                                server->close();
                            }
                        })
                        .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                        .onFinal([&]() -> void { clientFinal = true; });
                    const auto endpoint = *server->localEndpoint();
                    client->connect(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()});
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (serverFinal && clientFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 200"), 3U);
        REQUIRE_EQUAL(occurrences(responseWire, "HTTP/1.1 400"), 1U);
        REQUIRE(responseWire.find("abc") != std::string::npos);
        REQUIRE(responseWire.find("hello") != std::string::npos);
        REQUIRE(responseWire.find("{\"id\":12}") != std::string::npos);
    }

    void testAutomaticRouteLimitAndMediaErrors() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto clients = std::vector<TcpConnectionPtr>{};
        auto responses = std::vector<std::string>(2U);
        auto error = std::optional<NetworkErrorContext>{};
        auto clientFinalCount = std::size_t{};
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            auto options = HttpServerRouteOptions{};
            options.setMaximumBodyLength(el::unit::ByteLength{3U});
            server->events()
                .onTextRequest(
                    HttpMethod{HttpMethodType::Post},
                    "/text"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::text::String body) -> void {
                        request->sendText(std::move(body));
                    },
                    options)
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto requests = std::array{
                        bytes(
                            "POST /text HTTP/1.1\r\nHost: local\r\nContent-Type: text/plain\r\n"
                            "Content-Length: 4\r\nConnection: close\r\n\r\ntool"),
                        bytes(
                            "POST /text HTTP/1.1\r\nHost: local\r\nContent-Type: application/json\r\n"
                            "Content-Length: 1\r\nConnection: close\r\n\r\nx")};
                    for (auto index = std::size_t{}; index < requests.size(); ++index) {
                        auto client = loop->get<Network>().createTcpConnection();
                        client->events()
                            .onConnected([this, client, request = requests[index]]() -> void {
                                REQUIRE(client->send(request).isAccepted());
                            })
                            .onData([&, index](el::mem::ByteBlock data) -> void { responses[index] += raw(data); })
                            .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                            .onFinal([&]() -> void {
                                ++clientFinalCount;
                                if (clientFinalCount == 2U) {
                                    server->close();
                                }
                            });
                        const auto endpoint = *server->localEndpoint();
                        client->connect(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()});
                        clients.emplace_back(std::move(client));
                    }
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return serverFinal || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(clientFinalCount, 2U);
        REQUIRE(responses[0].find("HTTP/1.1 413") != std::string::npos);
        REQUIRE(responses[1].find("HTTP/1.1 415") != std::string::npos);
    }

    void testHttpsAlpnAndCookieSecurity() {
        SKIP_BY_DEFAULT()
        TAGS(FullRun)
        const auto applicationScope = ApplicationTestScope<>{};
        registerTlsConfigurations();
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto client = TlsClientConnectionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto connectionInfo = std::optional<HttpConnectionInfo>{};
        auto responseInfoSecure = false;
        auto responseWire = std::string{};
        auto serverFinal = false;
        auto clientFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->enableTls();
            server->setSessionManager(HttpCookieSessionManager::create());
            server->events()
                .onConnectionActive([&](const HttpConnectionInfo &info) -> void { connectionInfo = info; })
                .onNewSession([&](HttpServerSessionPtr session) -> void {
                    session->events().onRequestReceived([&](HttpServerRequestPtr request) -> void {
                        REQUIRE(request->connectionInfo().isSecure());
                        const auto weakRequest = std::weak_ptr<HttpServerRequest>{request};
                        request->events().onResponseCommitted([&, weakRequest](const HttpResponseHead &) -> void {
                            if (const auto retainedRequest = weakRequest.lock()) {
                                responseInfoSecure = retainedRequest->connectionInfo().isSecure();
                            }
                        });
                    });
                })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/secure"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendText("secure"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    client = loop->get<Network>().createTlsClientConnection();
                    client->events()
                        .onHandshakeCompleted([&]() -> void {
                            REQUIRE_EQUAL(client->negotiatedAlpn(), "http/1.1"_el);
                            REQUIRE(client
                                    ->send(
                                        bytes("GET /secure HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"))
                                    .isAccepted());
                        })
                        .onData([&](el::mem::ByteBlock data) -> void { responseWire += raw(data); })
                        .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                        .onFinal([&]() -> void {
                            clientFinal = true;
                            server->close();
                        });
                    auto clientOptions = TlsClientConnectOptions{};
                    clientOptions.setAlpnProtocols({"http/1.1"_el});
                    client->connect(
                        HostEndpoint{Host::fromStringOrThrow("localhost"_el), server->localEndpoint()->port()},
                        std::move(clientOptions));
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (serverFinal && clientFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE(responseWire.find("HTTP/1.1 200") != std::string::npos);
        REQUIRE(responseWire.find("Set-Cookie: erbsland-session=") != std::string::npos);
        REQUIRE(responseWire.find("; Secure") != std::string::npos);
        REQUIRE(responseWire.find("secure") != std::string::npos);
        REQUIRE(connectionInfo.has_value());
        REQUIRE(connectionInfo->isSecure());
        REQUIRE(connectionInfo->tls().has_value());
        REQUIRE_EQUAL(connectionInfo->tls()->negotiatedAlpn(), "http/1.1"_el);
        REQUIRE(connectionInfo->tls()->cipherSuite().has_value());
        REQUIRE(connectionInfo->tls()->signatureScheme().has_value());
        REQUIRE(responseInfoSecure);
    }

    void testHttpsTcpConnectionAbandonedBeforeHandshakeIsNormal() {
        SKIP_BY_DEFAULT()
        TAGS(FullRun)
        const auto applicationScope = ApplicationTestScope<>{};
        registerTlsConfigurations();
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto client = TcpConnectionPtr{};
        auto listenerError = std::optional<NetworkErrorContext>{};
        auto connectionError = std::optional<NetworkErrorContext>{};
        auto connectionActive = false;
        auto clientFinal = false;
        auto connectionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->enableTls();
            server->events()
                .onConnectionActive([&](const HttpConnectionInfo &) -> void { connectionActive = true; })
                .onConnectionError([&](const HttpConnectionInfo &, const NetworkErrorContext &context) -> void {
                    connectionError = context;
                })
                .onConnectionFinal([&](const HttpConnectionInfo &) -> void {
                    connectionFinal = true;
                    server->close();
                })
                .onError([&](const NetworkErrorContext &context) -> void { listenerError = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    client = loop->get<Network>().createTcpConnection();
                    client->events()
                        .onConnected([&]() -> void { client->close(); })
                        .onError([](const NetworkErrorContext &) -> void {})
                        .onFinal([&]() -> void { clientFinal = true; });
                    const auto endpoint = *server->localEndpoint();
                    client->connect(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()});
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (serverFinal && clientFinal) || listenerError.has_value(); });
        REQUIRE_FALSE(listenerError.has_value());
        REQUIRE_FALSE(connectionError.has_value());
        REQUIRE_FALSE(connectionActive);
        REQUIRE(clientFinal);
        REQUIRE(connectionFinal);
        REQUIRE(serverFinal);
    }

    void testHttpsAbruptClientCloseAfterResponseIsNormal() {
        SKIP_BY_DEFAULT()
        TAGS(FullRun)
        const auto applicationScope = ApplicationTestScope<>{};
        registerTlsConfigurations();
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto client = TlsClientConnectionPtr{};
        auto listenerError = std::optional<NetworkErrorContext>{};
        auto connectionError = std::optional<NetworkErrorContext>{};
        auto responseWire = std::string{};
        auto clientAbortRequested = false;
        auto clientFinal = false;
        auto connectionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->enableTls();
            server->events()
                .onConnectionError([&](const HttpConnectionInfo &, const NetworkErrorContext &context) -> void {
                    connectionError = context;
                })
                .onConnectionFinal([&](const HttpConnectionInfo &) -> void {
                    connectionFinal = true;
                    server->close();
                })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/complete"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendText("complete"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { listenerError = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    client = loop->get<Network>().createTlsClientConnection();
                    client->events()
                        .onHandshakeCompleted([&]() -> void {
                            REQUIRE(
                                client->send(bytes("GET /complete HTTP/1.1\r\nHost: localhost\r\n\r\n")).isAccepted());
                        })
                        .onData([&](el::mem::ByteBlock data) -> void {
                            responseWire += raw(data);
                            if (!clientAbortRequested && responseWire.find("\r\n\r\ncomplete") != std::string::npos) {
                                clientAbortRequested = true;
                                client->abort();
                            }
                        })
                        .onError([&](const NetworkErrorContext &context) -> void { listenerError = context; })
                        .onFinal([&]() -> void { clientFinal = true; });
                    auto clientOptions = TlsClientConnectOptions{};
                    clientOptions.setAlpnProtocols({"http/1.1"_el});
                    client->connect(
                        HostEndpoint{Host::fromStringOrThrow("localhost"_el), server->localEndpoint()->port()},
                        std::move(clientOptions));
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (serverFinal && clientFinal) || listenerError.has_value(); });
        REQUIRE_FALSE(listenerError.has_value());
        REQUIRE_FALSE(connectionError.has_value());
        REQUIRE(clientAbortRequested);
        REQUIRE(clientFinal);
        REQUIRE(connectionFinal);
        REQUIRE(serverFinal);
    }
};
