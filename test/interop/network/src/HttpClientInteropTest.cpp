// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InteropEnvironment.hpp"

#include "core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/HttpClientRequest.hpp>
#include <erbsland/HttpClientResponse.hpp>
#include <erbsland/HttpClientSession.hpp>
#include <erbsland/json/JsonValue.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/TlsConfiguration.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/Url.hpp>
#include <erbsland/X509CertificateBundle.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <chrono>
#include <optional>
#include <span>
#include <string_view>

using namespace el::cryptology;
using namespace el::event;
using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HttpClientSession HttpClientRequest HttpClientResponse)
class HttpClientInteropTest final : public el::UnitTest {
private:
    struct Result final {
        HttpClientResponsePtr response;
        std::optional<NetworkErrorContext> error;
        std::size_t informationalCount{};
        std::size_t requestFinalCount{};
        bool jsonReceived{};
        bool sessionFinal{};
    };

private:
    [[nodiscard]] static auto bytes(const std::string_view text) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromSpan(std::span<const char>{text.data(), text.size()});
    }

    static void installTlsConfiguration() {
        const auto ca = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/ca.pem")});
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            "http"_el, TlsConfiguration{TlsClientConfiguration{TlsTrustConfiguration{ca}}});
    }

    auto runScenario(const bool secure) -> Result {
        auto peer =
            InteropEnvironment::instance().startScenario(secure ? "http_tls_server" : "http_plain_server", "", 0U);
        const auto applicationScope = ApplicationTestScope<>{};
        installTlsConfiguration();
        const auto loop = EventLoop::create();
        auto session = HttpClientSessionPtr{};
        auto result = Result{};
        loop->invoke([&]() -> void {
            session = loop->get<Network>().createHttpClientSession();
            session->events()
                .onJsonResponse(
                    [&](HttpClientRequestPtr, HttpClientResponsePtr response, el::data::json::JsonValue body) -> void {
                        result.response = std::move(response);
                        result.jsonReceived = body.getOrThrow("ok"_el).getBoolOrThrow();
                        session->close();
                    })
                .onInformationalResponse([&](HttpClientRequestPtr, const HttpResponseHead &head) -> void {
                    REQUIRE_EQUAL(head.status().code(), 103U);
                    ++result.informationalCount;
                })
                .onError(
                    [&](HttpClientRequestPtr, const NetworkErrorContext &context) -> void { result.error = context; })
                .onFinal([&]() -> void { result.sessionFinal = true; });
            const auto endpoint = HostEndpoint{HostName::fromStringOrThrow("localhost"_el), Port{peer.port()}};
            const auto url = Url{secure ? UrlScheme::Https : UrlScheme::Http, endpoint, "/interop"_el, "x=1"_el};
            const auto request = session->sendPost(url, bytes("payload"));
            request->events().onFinal([&]() -> void { ++result.requestFinalCount; });
        });
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{20};
        while (!result.sessionFinal && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(el::time::TimeDelta::milliseconds(20)));
        }
        REQUIRE(result.sessionFinal);
        REQUIRE_EQUAL(peer.finish(), 7U);
        return result;
    }

    void verify(const Result &result) {
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE(result.jsonReceived);
        REQUIRE_EQUAL(result.informationalCount, 1U);
        REQUIRE_EQUAL(result.requestFinalCount, 1U);
        REQUIRE(result.response);
        REQUIRE_EQUAL(result.response->trailers().getFirst("X-Interop"_el), "rust"_el);
    }

public:
    void testIndependentPlaintextHttpServer() { verify(runScenario(false)); }

    void testIndependentRustlsHttpServer() { verify(runScenario(true)); }
};
