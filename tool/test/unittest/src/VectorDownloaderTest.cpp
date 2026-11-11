// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/HttpServer.hpp>
#include <erbsland/HttpServerRequest.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/UnmanagedEventThread.hpp>
#include <tool/test_data/impl/VectorDownloader.hpp>

#include <future>

TESTED_TARGETS(VectorDownloader)
class VectorDownloaderTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testLocalDownloadsRedirectsErrorsLimitsAndDeadlines() {
        runInsideApplication([&] {
            const auto thread = el::UnmanagedEventThread::create();
            auto server = el::HttpServerPtr{};
            auto endpoint = std::promise<el::IpEndpoint>{};
            auto ready = endpoint.get_future();
            thread->loop()->invoke([&] {
                server = thread->loop()->get<el::Network>().createHttpServer();
                server->events().onListening([&] { endpoint.set_value(*server->localEndpoint()); });
                server->events().onRequest(
                    el::HttpMethod{el::HttpMethodType::Get},
                    "/ok"_el,
                    [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) {
                        request->sendText("download\r\n"_el);
                    });
                server->events().onRequest(
                    el::HttpMethod{el::HttpMethodType::Get},
                    "/redirect"_el,
                    [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) {
                        request->sendRedirect("/ok"_el);
                    });
                server->events().onRequest(
                    el::HttpMethod{el::HttpMethodType::Get},
                    "/loop"_el,
                    [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) {
                        request->sendRedirect("/loop"_el);
                    });
                server->events().onRequest(
                    el::HttpMethod{el::HttpMethodType::Get},
                    "/error"_el,
                    [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) {
                        request->sendError(el::HttpStatus::NotFound);
                    });
                server->events().onRequest(
                    el::HttpMethod{el::HttpMethodType::Get},
                    "/idle"_el,
                    [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) {
                        // Keep the response open past the client's overall deadline.
                        request->startResponse(
                            el::HttpResponseHead{el::HttpVersion::Http11, el::HttpStatus::Ok, "OK"_el, {}});
                    });
                server->events().onRequest(
                    el::HttpMethod{el::HttpMethodType::Get},
                    "/interrupted"_el,
                    [&](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) {
                        auto headers = el::HttpHeaders{};
                        headers.addField(el::HttpFieldType::ContentLength, "100"_el);
                        request->startResponse(
                            el::HttpResponseHead{el::HttpVersion::Http11, el::HttpStatus::Ok, "OK"_el, headers});
                        const auto partial =
                            el::StringEncoder{el::String{"partial"_el}}.encode(el::StringEncoding::Utf8);
                        REQUIRE(request->sendBody(partial).isAccepted());
                        thread->loop()->invokeAfter(el::TimeDelta::milliseconds(10), [&] { server->abort(); });
                    });
                server->start(el::IpEndpoint{el::IpAddress::loopbackV4(), el::Port{}});
            });
            thread->start();
            REQUIRE(ready.wait_for(std::chrono::seconds{5}) == std::future_status::ready);
            const auto address = ready.get();
            const auto url = [&](const el::String &path) {
                return el::Url{el::UrlScheme::Http, el::HostEndpoint{address.address(), address.port()}, path};
            };
            auto downloader = tool::test_data::impl::VectorDownloader{el::ByteCount{1024}, el::TimeDelta::seconds(2)};
            const auto expected = el::StringEncoder{el::String{"download\r\n"_el}}.encode(el::StringEncoding::Utf8);
            REQUIRE_EQUAL(downloader.download(url("/ok"_el)), expected);
            REQUIRE_EQUAL(downloader.download(url("/redirect"_el)), expected);
            REQUIRE_THROWS(downloader.download(url("/loop"_el)));
            REQUIRE_THROWS(downloader.download(url("/error"_el)));
            auto small = tool::test_data::impl::VectorDownloader{el::ByteCount{2}, el::TimeDelta::seconds(2)};
            REQUIRE_THROWS(small.download(url("/ok"_el)));
            auto impatient =
                tool::test_data::impl::VectorDownloader{el::ByteCount{1024}, el::TimeDelta::milliseconds(100)};
            REQUIRE_THROWS(impatient.download(url("/idle"_el)));
            REQUIRE_THROWS(downloader.download(url("/interrupted"_el)));
            thread->loop()->invoke([&] { server->abort(); });
            thread->quit();
            thread->join();
            REQUIRE_FALSE(thread->workerEventLoop().hasError());
        });
    }
};
