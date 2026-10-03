// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VectorDownloader.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/HttpClientRequest.hpp>
#include <erbsland/HttpClientResponse.hpp>
#include <erbsland/HttpClientTlsOptions.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkError.hpp>
#include <erbsland/StringFormat.hpp>

#include <exception>

namespace tool::test_data::impl {

using namespace el::text::literals;

VectorDownloader::VectorDownloader(const el::ByteCount maximumBytes, const el::TimeDelta timeout) :
    _loop{el::EventLoop::create()}, _maximumBytes{maximumBytes} {
    _loop->invoke([this, maximumBytes, timeout]() -> void {
        _session = _loop->get<el::Network>().createHttpClientSession();
        // Bound transient transport queues independently of the complete source body.
        const auto receiveLimit = el::ByteCount{4U * 1024U * 1024U};
        _session->setTlsOptions(
            el::HttpClientTlsOptions{}.setBufferLimits(el::SocketBufferLimits{el::ByteCount{65536}, receiveLimit}));
        _session->setRedirectOptions(el::HttpClientRedirectOptions{}.setMaximumRedirects(el::ItemCount{5}));
        _session->setOptions(
            el::HttpClientSessionOptions{}
                .setSocketBufferLimits(el::SocketBufferLimits{el::ByteCount{65536}, el::ByteCount{65536}})
                .setMaximumBodyLength(maximumBytes)
                .setMaximumQueueLength(receiveLimit)
                .setOverallTimeout(timeout)
                .setMaximumConcurrentRequests(el::ItemCount{1}));
    });
    const auto initialized = _loop->runOnce();
    if (_loop->hasError()) {
        std::rethrow_exception(_loop->takeError());
    }
    if (!initialized || !_session) {
        throw el::ApplicationError{"Source Download Failed"_el, "The HTTP session could not be initialized."_el};
    }
}

VectorDownloader::~VectorDownloader() {
    if (_session) {
        _loop->invoke([this]() -> void { _session->abort(); });
        _loop->quit();
        _loop->run();
    }
}

auto VectorDownloader::download(const el::Url &url) -> el::ByteBlock {
    const auto result = std::make_shared<Download>();
    _loop->invoke([this, result, url]() -> void {
        const auto request = _session->createRequest(url);
        request->events().onResponse(
            [result, url](el::HttpClientRequestPtr, el::HttpClientResponsePtr response, el::ByteBlock body) -> void {
                if (!response->head().status().isSuccessful()) {
                    result->error = std::make_exception_ptr(
                        el::ApplicationError{
                            "Source Download Failed"_el,
                            el::StringFormat{"The server returned {}."_el}.build(response->head().status().toString())}
                            .withSourcePath(url.toString()));
                } else {
                    result->data = std::move(body);
                }
            },
            el::HttpClientResponseOptions{}.setMaximumBodyLength(_maximumBytes));
        request->events().onError(
            [result, url](el::HttpClientRequestPtr, const el::NetworkErrorContext &context) -> void {
                result->error = std::make_exception_ptr(
                    el::ApplicationError{
                        "Source Download Failed"_el,
                        "Could not download the configured source."_el,
                        std::make_exception_ptr(el::NetworkError{context})}
                        .withSourcePath(url.toString()));
            });
        request->events().onFinal([result]() -> void { result->finished = true; });
        _session->sendRequest(request);
    });
    while (!result->finished) {
        const auto processed = _loop->runOnce();
        if (_loop->hasError()) {
            std::rethrow_exception(_loop->takeError());
        }
        if (!processed && !result->finished) {
            throw el::ApplicationError{"Source Download Interrupted"_el, "The event loop stopped before completion."_el}
                .withSourcePath(url.toString());
        }
    }
    if (result->error) {
        std::rethrow_exception(result->error);
    }
    return std::move(result->data);
}

}
