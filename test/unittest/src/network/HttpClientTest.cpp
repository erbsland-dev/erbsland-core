// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"
#include "../path/PathTestFixture.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/network/http/HttpFieldType.hpp>
#include <erbsland/network/http_client/HttpClientRequest.hpp>
#include <erbsland/network/http_client/HttpClientResponse.hpp>
#include <erbsland/network/http_client/HttpClientSession.hpp>
#include <erbsland/network/http_client/HttpClientSessionOptions.hpp>
#include <erbsland/network/http_server/HttpServer.hpp>
#include <erbsland/network/http_server/HttpServerRequest.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/source/Connection.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>
#include <erbsland/network/url/Url.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimeUnitTags.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HttpClientSession HttpClientRequest HttpClientResponse)
class HttpClientTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto bytes(const std::string_view text) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromSpan(std::span<const char>{text.data(), text.size()});
    }

    [[nodiscard]] static auto readText(const char *path) -> el::text::String {
        return el::text::String{el::unittest::fh::readDataText(path)};
    }

    static void sendStreamedResponse(
        const HttpServerRequestPtr &request, const el::mem::ByteBlock &chunk, const std::size_t chunkCount) {
        auto headers = HttpHeaders{};
        headers.setField(HttpFieldType::ContentType, "application/octet-stream"_el)
            .setField(
                HttpFieldType::ContentLength, el::text::String::fromInteger(chunk.length().toRawValue() * chunkCount));
        request->startResponse(HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el, std::move(headers)});
        const auto weakRequest = std::weak_ptr<el::network::HttpServerRequest>{request};
        const auto sent = std::make_shared<std::size_t>();
        const auto finished = std::make_shared<bool>();
        const auto pump = [weakRequest, chunk, sent, finished, chunkCount]() -> void {
            const auto activeRequest = weakRequest.lock();
            if (!activeRequest || *finished) {
                return;
            }
            while (*sent < chunkCount) {
                const auto status = activeRequest->sendBody(chunk);
                if (status.wouldBlock()) {
                    return;
                }
                if (status.isClosed()) {
                    return;
                }
                ++*sent;
            }
            *finished = true;
            activeRequest->finishBody();
        };
        request->events().onWritable(pump);
        pump();
    }

    static void registerTlsConfigurations() {
        auto serverConfiguration = el::cryptology::TlsConfiguration{};
        serverConfiguration.setServerIdentity(
            el::cryptology::TlsServerIdentity{
                el::cryptology::X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/server.pem")),
                el::cryptology::SigningPrivateKey::fromPemOrThrow(
                    readText("data/network/tls-interop/server-key.pem"))});
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            "http/server"_el, std::move(serverConfiguration));
        auto clientConfiguration = el::cryptology::TlsConfiguration{el::cryptology::X509ServerCertificatePolicy{
            el::cryptology::X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/ca.pem"))}};
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            "http/client"_el, std::move(clientConfiguration));
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
    void testPreparedRequestValidationAndAsynchronousQueueLimit() {
        const auto loop = EventLoop::create();
        auto session = HttpClientSessionPtr{};
        auto rejectedError = std::optional<NetworkErrorContext>{};
        auto rejectedFinal = false;
        auto sessionFinal = false;

        loop->invoke([&]() -> void {
            session = loop->get<Network>().createHttpClientSession();
            auto options = HttpClientSessionOptions{};
            options.setMaximumConcurrentRequests(el::unit::ItemCount::one())
                .setMaximumPendingRequests(el::unit::ItemCount{});
            session->setOptions(options);
            REQUIRE_THROWS(session->createRequest(Url{}));
            auto protectedHeaders = HttpHeaders{};
            protectedHeaders.addField(HttpFieldType::Host, "forbidden"_el);
            REQUIRE_THROWS(session->setDefaultHeaders(protectedHeaders));

            const auto url = Url::fromStringOrThrow("http://127.0.0.1:9/queue"_el);
            auto manualCookie = session->createRequest(url);
            auto manualCookieHeaders = HttpHeaders{};
            manualCookieHeaders.setField(HttpFieldType::Cookie, "caller=forbidden"_el);
            manualCookie->setHeaders(std::move(manualCookieHeaders));
            REQUIRE_THROWS(session->sendRequest(manualCookie));
            static_cast<void>(session->sendGet(url));
            const auto rejected = session->createRequest(url);
            rejected->events()
                .onError(
                    [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { rejectedError = context; })
                .onFinal([&]() -> void {
                    rejectedFinal = true;
                    session->abort();
                });
            session->events().onFinal([&]() -> void { sessionFinal = true; });
            session->sendRequest(rejected);
        });

        runUntil(loop, [&]() -> bool { return rejectedFinal; });
        REQUIRE(sessionFinal);
        REQUIRE(rejectedError.has_value());
        REQUIRE_EQUAL(rejectedError->reason(), NetworkErrorReason::ResourceLimitExceeded);
        REQUIRE_EQUAL(rejectedError->phase(), NetworkErrorPhase::HttpRequest);
    }

    void testPlaintextAutomaticTextPostAndGracefulDrain() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto request = HttpClientRequestPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto receivedText = el::text::String{};
        auto receivedHost = el::text::String{};
        auto requestFinalCount = std::size_t{};
        auto sessionClosed = false;
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Post},
                    "/submit"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr serverRequest, el::mem::ByteBlock body) -> void {
                        REQUIRE_EQUAL(body, bytes("request-body"));
                        receivedHost = serverRequest->head().headers().getFirst(HttpFieldType::Host);
                        serverRequest->sendText("response-text"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    const auto url =
                        Url{UrlScheme::Http,
                            HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()},
                            "/submit"_el,
                            ""_el,
                            "ignored-fragment"_el};
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onTextResponse(
                            [&](HttpClientRequestPtr callbackRequest,
                                HttpClientResponsePtr response,
                                el::text::String text) -> void {
                                REQUIRE_EQUAL(callbackRequest, request);
                                REQUIRE_EQUAL(response->head().status(), HttpStatus::Ok);
                                receivedText = std::move(text);
                                session->close();
                            })
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void {
                            sessionClosed = true;
                            server->close();
                        })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    request = session->sendPost(url, bytes("request-body"));
                    request->events().onFinal([&]() -> void { ++requestFinalCount; });
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(receivedText, "response-text"_el);
        REQUIRE_EQUAL(requestFinalCount, 1U);
        REQUIRE(sessionClosed);
        REQUIRE(sessionFinal);
        REQUIRE_FALSE(receivedHost.isEmpty());
        REQUIRE_FALSE(receivedHost.contains("ignored-fragment"_el));
    }

    void testStreamedUploadDefaultPrecedenceAndLowLevelTextResponse() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto request = HttpClientRequestPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto responseText = el::text::String{};
        auto responseFinalCount = std::size_t{};
        auto initialWouldBlock = false;
        auto uploadSent = false;
        auto uploadFinished = false;
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod::fromStringOrThrow("PATCH"_el),
                    "/stream"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr serverRequest, el::mem::ByteBlock body) -> void {
                        REQUIRE_EQUAL(serverRequest->head().target(), "/stream?"_el);
                        REQUIRE_EQUAL(body, bytes("stream-body"));
                        const auto values = serverRequest->head().headers().getAll("X-Test"_el);
                        REQUIRE_EQUAL(values.count(), el::unit::ItemCount{2U});
                        REQUIRE_EQUAL(values.get(el::unit::ItemIndex{}), "request-one"_el);
                        REQUIRE_EQUAL(values.get(el::unit::ItemIndex{1U}), "request-two"_el);
                        serverRequest->sendText("low-level-response"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    const auto urlText = el::text::String{el::text::StringEditor{"http://"_el}
                            .append(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()}.toString())
                            .append("/stream?#ignored"_el)};
                    session = loop->get<Network>().createHttpClientSession();
                    auto defaults = HttpHeaders{};
                    defaults.addField("X-Test"_el, "default-one"_el).addField("X-Test"_el, "default-two"_el);
                    session->setDefaultHeaders(std::move(defaults));
                    session->events()
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    request = session->createRequest(
                        HttpMethod::fromStringOrThrow("PATCH"_el), Url::fromStringOrThrow(urlText));
                    auto headers = HttpHeaders{};
                    headers.addField("X-Test"_el, "request-one"_el).addField("X-Test"_el, "request-two"_el);
                    request->setHeaders(std::move(headers));
                    request->streamBody();
                    request->events()
                        .onResponseHead([&](HttpClientRequestPtr, HttpClientResponsePtr response) -> void {
                            response->events()
                                .onText([&](el::text::String text) -> void {
                                    responseText = std::move(text);
                                    session->close();
                                })
                                .onFinal([&]() -> void { ++responseFinalCount; });
                            response->aggregateText(el::unit::ByteLength{1024U});
                        })
                        .onWritable([&]() -> void {
                            if (!uploadSent) {
                                REQUIRE(request->sendBody(bytes("stream-body")).isAccepted());
                                uploadSent = true;
                            }
                            if (!uploadFinished) {
                                auto trailers = HttpHeaders{};
                                trailers.addField("X-Upload-End"_el, "yes"_el);
                                const auto status = request->finishBody(std::move(trailers));
                                REQUIRE_FALSE(status.isClosed());
                                uploadFinished = status.isAccepted();
                            }
                        });
                    initialWouldBlock = request->sendBody(bytes("too-early")).wouldBlock();
                    session->sendRequest(request);
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE(initialWouldBlock);
        REQUIRE(uploadSent);
        REQUIRE(uploadFinished);
        REQUIRE_EQUAL(responseText, "low-level-response"_el);
        REQUIRE_EQUAL(responseFinalCount, 1U);
    }

    void testAutomaticContentTypeFailureTerminatesRequest() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto requestFinalCount = std::size_t{};
        auto responseCalled = false;
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/wrong-type"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr serverRequest, el::mem::ByteBlock) -> void {
                        auto headers = HttpHeaders{};
                        headers.setField(HttpFieldType::ContentType, "application/octet-stream"_el);
                        serverRequest->sendResponse(
                            HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el, std::move(headers)},
                            bytes("not-text-policy"));
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onTextResponse([&](HttpClientRequestPtr, HttpClientResponsePtr, el::text::String) -> void {
                            responseCalled = true;
                        })
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    const auto request = session->sendGet(
                        Url{UrlScheme::Http,
                            HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()},
                            "/wrong-type"_el});
                    request->events().onFinal([&]() -> void {
                        ++requestFinalCount;
                        session->close();
                    });
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return sessionFinal && serverFinal; });
        REQUIRE(error.has_value());
        REQUIRE_EQUAL(error->reason(), NetworkErrorReason::HttpResponseValidationFailure);
        REQUIRE_EQUAL(requestFinalCount, 1U);
        REQUIRE_FALSE(responseCalled);
    }

    void testRedirectCookiesAndSequentialConnectionReuse() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto firstConnection = ConnectionPtr{};
        auto redirectCalls = std::size_t{};
        auto responseCalls = std::size_t{};
        auto cookieSeen = false;
        auto reused = false;
        auto effectiveUrl = el::text::String{};
        auto redirectCount = el::unit::ItemCount{};
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/start"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr serverRequest, el::mem::ByteBlock) -> void {
                        firstConnection = serverRequest->connection();
                        auto headers = HttpHeaders{};
                        headers.addField(HttpFieldType::SetCookie, "redirect-token=accepted; Path=/; HttpOnly"_el);
                        serverRequest->sendRedirect("/target#redirect-fragment"_el, HttpStatus::Found, headers);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/target"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr serverRequest, el::mem::ByteBlock) -> void {
                        cookieSeen = serverRequest->head().headers().getFirst(HttpFieldType::Cookie) ==
                            "redirect-token=accepted"_el;
                        reused = serverRequest->connection() == firstConnection;
                        serverRequest->sendText("target"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/again"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr serverRequest, el::mem::ByteBlock) -> void {
                        reused = reused && serverRequest->connection() == firstConnection;
                        serverRequest->sendText("again"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    const auto host = HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()};
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onRedirect(
                            [&](HttpClientRequestPtr, HttpClientRedirectContext &context) -> HttpClientRedirectAction {
                                ++redirectCalls;
                                REQUIRE(context.canFollow());
                                REQUIRE(context.targetUrl().has_value());
                                return HttpClientRedirectAction::Follow;
                            })
                        .onTextResponse(
                            [&](HttpClientRequestPtr, HttpClientResponsePtr response, el::text::String text) -> void {
                                ++responseCalls;
                                if (text == "target"_el) {
                                    effectiveUrl = response->effectiveUrl().toString();
                                    redirectCount = response->redirectCount();
                                } else {
                                    REQUIRE_EQUAL(text, "again"_el);
                                    session->close();
                                }
                            })
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    const auto request =
                        session->sendGet(Url{UrlScheme::Http, host, "/start"_el, {}, "original-fragment"_el});
                    request->events().onFinal([&, host]() -> void {
                        static_cast<void>(session->sendGet(Url{UrlScheme::Http, host, "/again"_el}));
                    });
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(redirectCalls, 1U);
        REQUIRE_EQUAL(responseCalls, 2U);
        REQUIRE(cookieSeen);
        REQUIRE(reused);
        REQUIRE_EQUAL(redirectCount, el::unit::ItemCount::one());
        REQUIRE(effectiveUrl.endsWith("/target#redirect-fragment"_el));
    }

    void testRedirectStatusAndMethodRewriteMatrix() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto redirectCalls = std::size_t{};
        auto responseCalls = std::size_t{};
        auto requestFinalCalls = std::size_t{};
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    "/redirect/{case}"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        const auto name = *request->parameter("case"_el);
                        auto status = HttpStatus::Found;
                        if (name.startsWith("301-"_el)) {
                            status = HttpStatus::MovedPermanently;
                        } else if (name.startsWith("303-"_el)) {
                            status = HttpStatus::SeeOther;
                        } else if (name.startsWith("307-"_el)) {
                            status = HttpStatus::TemporaryRedirect;
                        } else if (name.startsWith("308-"_el)) {
                            status = HttpStatus::PermanentRedirect;
                        }
                        request->sendRedirect(el::text::String::fromJoined({"/target/"_el, name}), status);
                    })
                .onRequest(
                    "/target/{case}"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock body) -> void {
                        const auto name = *request->parameter("case"_el);
                        const auto rewritten = name == "301-post"_el || name == "302-post"_el ||
                            name == "303-post"_el || name == "303-put"_el;
                        const auto expectedMethod = rewritten ? HttpMethodType::Get
                            : name.endsWith("-head"_el)       ? HttpMethodType::Head
                            : name.endsWith("-put"_el)        ? HttpMethodType::Put
                                                              : HttpMethodType::Post;
                        REQUIRE_EQUAL(request->head().method().standardType(), expectedMethod);
                        REQUIRE_EQUAL(body.isEmpty(), rewritten || expectedMethod == HttpMethodType::Head);
                        REQUIRE_EQUAL(request->head().headers().hasField(HttpFieldType::ContentType), !rewritten);
                        request->sendText("ok"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    const auto host = HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()};
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onRedirect(
                            [&](HttpClientRequestPtr, HttpClientRedirectContext &context) -> HttpClientRedirectAction {
                                ++redirectCalls;
                                REQUIRE(context.canFollow());
                                return HttpClientRedirectAction::Follow;
                            })
                        .onTextResponse(
                            [&](HttpClientRequestPtr, HttpClientResponsePtr response, el::text::String body) -> void {
                                ++responseCalls;
                                REQUIRE(body == "ok"_el || body.isEmpty());
                                REQUIRE_EQUAL(response->redirectCount(), el::unit::ItemCount::one());
                                if (responseCalls == 9U) {
                                    session->close();
                                }
                            })
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    const auto scenarios = std::array{
                        std::pair{HttpMethodType::Post, "301-post"_el},
                        std::pair{HttpMethodType::Put, "301-put"_el},
                        std::pair{HttpMethodType::Post, "302-post"_el},
                        std::pair{HttpMethodType::Put, "302-put"_el},
                        std::pair{HttpMethodType::Post, "303-post"_el},
                        std::pair{HttpMethodType::Put, "303-put"_el},
                        std::pair{HttpMethodType::Head, "303-head"_el},
                        std::pair{HttpMethodType::Post, "307-post"_el},
                        std::pair{HttpMethodType::Post, "308-post"_el}};
                    for (const auto &[method, name] : scenarios) {
                        auto request = session->createRequest(
                            HttpMethod{method},
                            Url{UrlScheme::Http, host, el::text::String::fromJoined({"/redirect/"_el, name})});
                        auto headers = HttpHeaders{};
                        headers.setField(HttpFieldType::ContentType, "text/plain"_el);
                        request->setHeaders(std::move(headers));
                        if (method != HttpMethodType::Head) {
                            request->setBody(bytes("payload"));
                        }
                        request->events().onFinal([&]() -> void { ++requestFinalCalls; });
                        session->sendRequest(request);
                    }
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(redirectCalls, 9U);
        REQUIRE_EQUAL(responseCalls, 9U);
        REQUIRE_EQUAL(requestFinalCalls, 9U);
    }

    void testRedirectActionsGuardsAndStreamReplayability() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto expectedErrors = std::size_t{};
        auto sessionRedirectCalls = std::size_t{};
        auto requestRedirectCalls = std::size_t{};
        auto overrideRedirectCalls = std::size_t{};
        auto responseCalls = std::size_t{};
        auto finalCalls = std::size_t{};
        auto uploadFinished = false;
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/return"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendRedirect("/target"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/reject"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendRedirect("/target"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/loop"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendRedirect("/loop#changed"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/malformed"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendRedirect("http://[invalid"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/limit-one"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendRedirect("/limit-two"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/limit-two"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendRedirect("/target"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Post},
                    "/stream"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendRedirect("/target"_el, HttpStatus::TemporaryRedirect);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/override"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendRedirect("/override-target"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Put},
                    "/override-target"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock body) -> void {
                        REQUIRE_EQUAL(body, bytes("override"));
                        request->sendText("override-ok"_el);
                    })
                .onRequest(
                    "/target"_el,
                    [](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        request->sendText("unexpected"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    const auto host = HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()};
                    const auto makeUrl = [host](const el::text::String &path) -> Url {
                        return Url{UrlScheme::Http, host, path};
                    };
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onRedirect(
                            [&](HttpClientRequestPtr request, HttpClientRedirectContext &) -> HttpClientRedirectAction {
                                ++sessionRedirectCalls;
                                return request->url().path() == "/reject"_el ? HttpClientRedirectAction::Reject
                                                                             : HttpClientRedirectAction::Follow;
                            })
                        .onResponse(
                            [&](HttpClientRequestPtr, HttpClientResponsePtr response, el::mem::ByteBlock body) -> void {
                                ++responseCalls;
                                if (response->head().status() == HttpStatus::Found) {
                                    REQUIRE(body.isEmpty());
                                    REQUIRE_EQUAL(response->redirectCount(), el::unit::ItemCount{});
                                } else {
                                    REQUIRE_EQUAL(body, bytes("override-ok"));
                                    REQUIRE_EQUAL(response->redirectCount(), el::unit::ItemCount::one());
                                }
                            })
                        .onError([&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void {
                            REQUIRE_EQUAL(context.reason(), NetworkErrorReason::HttpRedirectFailure);
                            ++expectedErrors;
                        })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    const auto track = [&](const HttpClientRequestPtr &request) -> void {
                        request->events().onFinal([&]() -> void {
                            ++finalCalls;
                            if (finalCalls == 7U) {
                                session->close();
                            }
                        });
                    };

                    auto returned = session->createRequest(makeUrl("/return"_el));
                    returned->events().onRedirect(
                        [&](HttpClientRequestPtr, HttpClientRedirectContext &context) -> HttpClientRedirectAction {
                            ++requestRedirectCalls;
                            REQUIRE(context.canFollow());
                            return HttpClientRedirectAction::ReturnResponse;
                        });
                    track(returned);
                    session->sendRequest(returned);

                    for (const auto path : {"/reject"_el, "/loop"_el, "/malformed"_el}) {
                        auto request = session->createRequest(makeUrl(path));
                        track(request);
                        session->sendRequest(request);
                    }

                    auto limited = session->createRequest(makeUrl("/limit-one"_el));
                    limited->setRedirectOptions(
                        HttpClientRedirectOptions{}.setMaximumRedirects(el::unit::ItemCount::one()));
                    track(limited);
                    session->sendRequest(limited);

                    auto streamed = session->createRequest(HttpMethodType::Post, makeUrl("/stream"_el));
                    streamed->streamBody();
                    const auto weakStreamed = std::weak_ptr<el::network::HttpClientRequest>{streamed};
                    streamed->events().onWritable([&, weakStreamed]() -> void {
                        if (const auto active = weakStreamed.lock(); active && !uploadFinished) {
                            REQUIRE(active->sendBody(bytes("streamed")).isAccepted());
                            uploadFinished = active->finishBody().isAccepted();
                        }
                    });
                    track(streamed);
                    session->sendRequest(streamed);

                    auto overridden = session->createRequest(makeUrl("/override"_el));
                    overridden->events().onRedirect(
                        [&](HttpClientRequestPtr, HttpClientRedirectContext &context) -> HttpClientRedirectAction {
                            ++overrideRedirectCalls;
                            auto replacement = session->createRequest(HttpMethodType::Put, *context.targetUrl());
                            replacement->setBody(bytes("override"));
                            context.setRequestOverride(std::move(replacement));
                            return HttpClientRedirectAction::Follow;
                        });
                    track(overridden);
                    session->sendRequest(overridden);
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE(uploadFinished);
        REQUIRE_EQUAL(requestRedirectCalls, 1U);
        REQUIRE_EQUAL(overrideRedirectCalls, 1U);
        REQUIRE_EQUAL(sessionRedirectCalls, 6U);
        REQUIRE_EQUAL(responseCalls, 2U);
        REQUIRE_EQUAL(expectedErrors, 5U);
        REQUIRE_EQUAL(finalCalls, 7U);
    }

    void testCrossOriginRedirectStripsSensitiveFieldsAndEnforcesHostPolicies() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto redirectLocation = el::text::String{};
        auto responseCalls = std::size_t{};
        auto policyErrors = std::size_t{};
        auto finalCalls = std::size_t{};
        auto sourceConnection = ConnectionPtr{};
        auto targetConnection = ConnectionPtr{};
        auto stripped = false;
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/cross"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        sourceConnection = request->connection();
                        request->sendRedirect(redirectLocation);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/target"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        targetConnection = request->connection();
                        const auto &headers = request->head().headers();
                        stripped = !headers.hasField(HttpFieldType::Authorization) &&
                            !headers.hasField(HttpFieldType::ProxyAuthorization) &&
                            !headers.hasField(HttpFieldType::Cookie) && !headers.hasField(HttpFieldType::Referer) &&
                            !headers.hasField("Origin"_el);
                        request->sendText("crossed"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    const auto targetHost = HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()};
                    redirectLocation = el::text::String{
                        el::text::StringEditor{"http://"_el}.append(targetHost.toString()).append("/target"_el)};
                    const auto sourceHost = HostEndpoint{Host::fromStringOrThrow("localhost"_el), endpoint.port()};
                    const auto sourceUrl = Url{UrlScheme::Http, sourceHost, "/cross"_el};
                    session = loop->get<Network>().createHttpClientSession();
                    auto options = HttpClientSessionOptions{};
                    options.setAutomaticCookiesEnabled(false);
                    session->setOptions(options);
                    auto defaults = HttpHeaders{};
                    defaults.setField(HttpFieldType::Authorization, "Bearer secret"_el)
                        .setField("Origin"_el, "http://localhost"_el);
                    session->setDefaultHeaders(std::move(defaults));
                    session->events()
                        .onTextResponse(
                            [&](HttpClientRequestPtr, HttpClientResponsePtr, el::text::String body) -> void {
                                REQUIRE_EQUAL(body, "crossed"_el);
                                ++responseCalls;
                            })
                        .onError([&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void {
                            REQUIRE_EQUAL(context.reason(), NetworkErrorReason::HttpRedirectFailure);
                            ++policyErrors;
                        })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    const auto submit = [&](const HttpClientRedirectHostPolicy policy) -> void {
                        auto request = session->createRequest(sourceUrl);
                        request->setRedirectOptions(HttpClientRedirectOptions{}.setHostPolicy(policy));
                        auto headers = HttpHeaders{};
                        headers.setField(HttpFieldType::ProxyAuthorization, "Basic secret"_el)
                            .setField(HttpFieldType::Cookie, "caller=secret"_el)
                            .setField(HttpFieldType::Referer, "http://localhost/private"_el);
                        request->setHeaders(std::move(headers));
                        request->events().onFinal([&]() -> void {
                            ++finalCalls;
                            if (finalCalls == 3U) {
                                session->close();
                            }
                        });
                        session->sendRequest(request);
                    };
                    submit(HttpClientRedirectHostPolicy::Any);
                    submit(HttpClientRedirectHostPolicy::SameHost);
                    submit(HttpClientRedirectHostPolicy::SameRegistrableDomain);
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE(stripped);
        REQUIRE_NOT_EQUAL(sourceConnection, targetConnection);
        REQUIRE_EQUAL(responseCalls, 1U);
        REQUIRE_EQUAL(policyErrors, 2U);
        REQUIRE_EQUAL(finalCalls, 3U);
    }

    void testSequentialReuseOptionsAndConfigurationGeneration() {
        SKIP_BY_DEFAULT()
        TAGS(FullRun)
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto connections = std::vector<ConnectionPtr>{};
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    "/request/{index}"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        connections.emplace_back(request->connection());
                        request->sendText("ok"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    const auto host = HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()};
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onTextResponse(
                            [&](HttpClientRequestPtr, HttpClientResponsePtr, el::text::String body) -> void {
                                REQUIRE_EQUAL(body, "ok"_el);
                            })
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    const auto sendNext = std::make_shared<std::function<void(std::size_t)>>();
                    *sendNext = [&, host, sendNext](const std::size_t index) -> void {
                        auto path = el::text::StringEditor{};
                        path.append("/request/"_el).append(index);
                        auto request = session->sendGet(Url{UrlScheme::Http, host, el::text::String{path}});
                        request->events().onFinal([&, index, sendNext]() -> void {
                            if (index == 5U) {
                                session->close();
                                return;
                            }
                            const auto nextIndex = index + 1U;
                            if (nextIndex == 2U) {
                                auto options = HttpClientSessionOptions{};
                                options.setConnectionReuseEnabled(false);
                                session->setOptions(options);
                            } else if (nextIndex == 3U) {
                                auto options = HttpClientSessionOptions{};
                                options.setMaximumTransactionsPerConnection(el::unit::ItemCount::one());
                                session->setOptions(options);
                            } else if (nextIndex == 4U) {
                                session->setOptions(HttpClientSessionOptions{});
                            }
                            loop->invokeAfter(
                                el::time::TimeDelta{el::time::Milliseconds{10}},
                                [sendNext, nextIndex]() -> void { (*sendNext)(nextIndex); });
                        });
                    };
                    (*sendNext)(1U);
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return sessionFinal || error.has_value(); });
        if (!serverFinal) {
            server->abort();
        }
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(connections.size(), 5U);
        REQUIRE_NOT_EQUAL(connections[0], connections[1]);
        REQUIRE_NOT_EQUAL(connections[1], connections[2]);
        REQUIRE_NOT_EQUAL(connections[2], connections[3]);
        REQUIRE_EQUAL(connections[3], connections[4]);
    }

    void testStaleReusedConnectionRetriesOnlySafeRequests() {
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto firstConnection = ConnectionPtr{};
        auto safeRetryConnection = ConnectionPtr{};
        auto safeAttempts = std::size_t{};
        auto unsafeAttempts = std::size_t{};
        auto unsafeWasReused = false;
        auto safeResponseReceived = false;
        auto unsafeError = std::optional<NetworkErrorContext>{};
        auto serverError = std::optional<NetworkErrorContext>{};
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/prime"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        firstConnection = request->connection();
                        request->sendText("prime"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/safe"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        ++safeAttempts;
                        if (request->connection() == firstConnection) {
                            request->connection()->abort();
                            return;
                        }
                        safeRetryConnection = request->connection();
                        request->sendText("safe"_el);
                    })
                .onRequest(
                    HttpMethod{HttpMethodType::Post},
                    "/unsafe"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock body) -> void {
                        REQUIRE_EQUAL(body, bytes("fixed"));
                        ++unsafeAttempts;
                        unsafeWasReused = request->connection() == safeRetryConnection;
                        request->connection()->abort();
                    })
                .onError([&](const NetworkErrorContext &context) -> void { serverError = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    const auto host = HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()};
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onTextResponse(
                            [&](HttpClientRequestPtr, HttpClientResponsePtr, el::text::String body) -> void {
                                if (body == "prime"_el) {
                                    return;
                                }
                                REQUIRE_EQUAL(body, "safe"_el);
                                safeResponseReceived = true;
                            })
                        .onError([&](HttpClientRequestPtr request, const NetworkErrorContext &context) -> void {
                            REQUIRE_EQUAL(request->url().path(), "/unsafe"_el);
                            unsafeError = context;
                            session->abort();
                        })
                        .onFinal([&]() -> void {
                            sessionFinal = true;
                            server->abort();
                        });
                    auto prime = session->sendGet(Url{UrlScheme::Http, host, "/prime"_el});
                    prime->events().onFinal([&, host]() -> void {
                        auto safe = session->sendGet(Url{UrlScheme::Http, host, "/safe"_el});
                        safe->events().onFinal([&, host]() -> void {
                            static_cast<void>(
                                session->sendPost(Url{UrlScheme::Http, host, "/unsafe"_el}, bytes("fixed")));
                        });
                    });
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return sessionFinal && serverFinal; });
        REQUIRE_FALSE(serverError.has_value());
        REQUIRE(safeResponseReceived);
        REQUIRE(unsafeError.has_value());
        REQUIRE_EQUAL(safeAttempts, 2U);
        REQUIRE_EQUAL(unsafeAttempts, 1U);
        REQUIRE(unsafeWasReused);
        REQUIRE_NOT_EQUAL(firstConnection, safeRetryConnection);
    }

    void testAtomicPathDownloadReplacesOnlyAfterSuccess() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = el::test::pathtest::PathTestFixture{"http-client-download"};
        const auto destination = fixture.child("result.txt");
        destination.content().writeTextOrThrow("previous"_el);
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto progress = std::vector<el::unit::ByteLength>{};
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/download"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        sendStreamedResponse(request, bytes(std::string(8000U, 'r')), 3U);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onResponseHead([&](HttpClientRequestPtr request, HttpClientResponsePtr response) -> void {
                            response->events().onBodyProgress([&](const HttpClientBodyProgress &value) -> void {
                                progress.emplace_back(value.committedLength());
                            });
                            response->writeBodyTo(destination);
                            request->events().onFinal([&]() -> void { session->close(); });
                        })
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    static_cast<void>(session->sendGet(
                        Url{UrlScheme::Http,
                            HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()},
                            "/download"_el}));
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE_FALSE(error.has_value()); },
            [&]() -> std::string {
                if (!error.has_value()) {
                    return {};
                }
                return el::text::StringConverter{
                    el::text::String::fromJoined({error->title(), ": "_el, error->description()})}
                    .toStdString();
            });
        const auto downloaded = destination.content().readTextOrThrow();
        REQUIRE_EQUAL(downloaded.length(), el::unit::ByteLength{24000U});
        REQUIRE(downloaded.startsWith("rrrrrrrr"_el));
        REQUIRE_FALSE(progress.empty());
        REQUIRE_EQUAL(progress.back(), el::unit::ByteLength{24000U});
        for (auto index = std::size_t{1U}; index < progress.size(); ++index) {
            REQUIRE(progress[index] >= progress[index - 1U]);
        }
    }

    void testCancelledPathDownloadPreservesDestinationAndCleansTemporaryFile() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = el::test::pathtest::PathTestFixture{"http-client-download-cancel"};
        const auto destination = fixture.child("result.txt");
        destination.content().writeTextOrThrow("previous"_el);
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto progressCalls = std::size_t{};
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/download"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr request, el::mem::ByteBlock) -> void {
                        sendStreamedResponse(request, bytes(std::string(8000U, 'x')), 3U);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    const auto endpoint = *server->localEndpoint();
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onResponseHead([&](HttpClientRequestPtr request, HttpClientResponsePtr response) -> void {
                            const auto weakRequest = std::weak_ptr<el::network::HttpClientRequest>{request};
                            response->events().onBodyProgress([&, weakRequest](const HttpClientBodyProgress &) {
                                ++progressCalls;
                                if (const auto activeRequest = weakRequest.lock()) {
                                    activeRequest->cancel();
                                }
                            });
                            response->writeBodyTo(destination);
                            request->events().onFinal([&]() -> void { session->close(); });
                        })
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    static_cast<void>(session->sendGet(
                        Url{UrlScheme::Http,
                            HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()},
                            "/download"_el}));
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE_FALSE(error.has_value()); },
            [&]() -> std::string {
                if (!error.has_value()) {
                    return {};
                }
                return std::string{"progress="} + std::to_string(progressCalls) + " | " +
                    el::text::StringConverter{
                        el::text::String::fromJoined({error->title(), ": "_el, error->description()})}
                        .toStdString();
            });
        REQUIRE_EQUAL(progressCalls, 1U);
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "previous"_el);
        const auto cleanupDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{50};
        while (
            std::distance(
                std::filesystem::directory_iterator{fixture.stdPath()}, std::filesystem::directory_iterator{}) != 1 &&
            std::chrono::steady_clock::now() < cleanupDeadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        REQUIRE_EQUAL(
            std::distance(
                std::filesystem::directory_iterator{fixture.stdPath()}, std::filesystem::directory_iterator{}),
            1);
    }

    void testHttpsWithHttpClientConfigurationAndForcedAlpn() {
        SKIP_BY_DEFAULT()
        TAGS(FullRun)
        const auto applicationScope = ApplicationTestScope<>{};
        registerTlsConfigurations();
        const auto loop = EventLoop::create();
        auto server = HttpServerPtr{};
        auto session = HttpClientSessionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto responseText = el::text::String{};
        auto sessionFinal = false;
        auto serverFinal = false;

        loop->invoke([&]() -> void {
            server = loop->get<Network>().createHttpServer();
            server->enableTls();
            server->events()
                .onRequest(
                    HttpMethod{HttpMethodType::Get},
                    "/secure"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr serverRequest, el::mem::ByteBlock) -> void {
                        serverRequest->sendText("secure-client"_el);
                    })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { serverFinal = true; })
                .onListening([&]() -> void {
                    session = loop->get<Network>().createHttpClientSession();
                    session->events()
                        .onTextResponse(
                            [&](HttpClientRequestPtr, HttpClientResponsePtr, el::text::String text) -> void {
                                responseText = std::move(text);
                                session->close();
                            })
                        .onError(
                            [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { error = context; })
                        .onClosed([&]() -> void { server->close(); })
                        .onFinal([&]() -> void { sessionFinal = true; });
                    const auto endpoint =
                        HostEndpoint{Host::fromStringOrThrow("localhost"_el), server->localEndpoint()->port()};
                    static_cast<void>(session->sendGet(Url{UrlScheme::Https, endpoint, "/secure"_el}));
                });
            server->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return (sessionFinal && serverFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(responseText, "secure-client"_el);
    }
};
