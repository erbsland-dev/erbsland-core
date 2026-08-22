// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRequest_fwd.hpp"
#include "HttpClientResponse_fwd.hpp"
#include "HttpClientResponseEventEditor_fwd.hpp"
#include "HttpClientResponseHandler.hpp"

#include "../codec/Http1Transaction_fwd.hpp"

#include "../../../../path/Path.hpp"
#include "../../../../stream/ByteOutputStream_fwd.hpp"
#include "../../../../stream/TempByteOutputStream_fwd.hpp"
#include "../../../http/HttpMediaType.hpp"
#include "../../../http_client/HttpClientResponse.hpp"
#include "../../../source/NetworkErrorReason.hpp"

#include <atomic>
#include <exception>
#include <memory>
#include <optional>

namespace erbsland::network::impl {

/// Built-in retained HTTP client response.
/// @tested{HttpClientTest}
class HttpClientResponse final : public network::HttpClientResponse {
    friend class HttpClientRequest;
    friend class HttpClientResponseEventEditor;

private:
    /// Selected post-aggregation conversion.
    enum class AggregateMode : std::uint8_t {
        None,  ///< No post-aggregation conversion.
        Bytes, ///< Deliver raw bytes.
        Text,  ///< Decode strict UTF-8.
        Json,  ///< Decode strict UTF-8 and parse JSON.
    };

public:
    /// Create one response at its paused final-head checkpoint.
    HttpClientResponse(
        std::shared_ptr<HttpClientRequest> request,
        HttpResponseHead head,
        Http1TransactionPtr transaction,
        bool hasBody,
        Url effectiveUrl,
        unit::ItemCount redirectCount,
        std::optional<unit::ByteLength> expectedLength);
    ~HttpClientResponse() override;

public: // implement network::HttpClientResponse
    [[nodiscard]] auto head() const noexcept -> const HttpResponseHead & override;
    [[nodiscard]] auto trailers() const noexcept -> const HttpHeaders & override;
    [[nodiscard]] auto effectiveUrl() const noexcept -> const Url & override;
    [[nodiscard]] auto redirectCount() const noexcept -> unit::ItemCount override;
    void streamBody() override;
    void aggregateBody(unit::ByteLength maximumLength) override;
    void aggregateText(unit::ByteLength maximumLength) override;
    void aggregateJson(unit::ByteLength maximumLength) override;
    void writeBodyTo(stream::ByteOutputStreamPtr output) override;
    void writeBodyTo(path::Path destination) override;
    void rejectBody() override;
    void pauseBody() override;
    void resumeBody() override;
    [[nodiscard]] auto isFinal() const noexcept -> bool override;
    [[nodiscard]] auto events() -> network::HttpClientResponseEventEditor & override;

private:
    /// Select automatic handling and start body processing.
    void startAutomatic(const HttpClientResponseHandler &handler);
    /// Deliver one transaction body block.
    void handleBodyData(mem::ByteBlock data);
    /// Deliver one completed aggregate.
    void handleAggregatedBody(mem::ByteBlock data);
    /// Capture trailer fields.
    void handleTrailers(const HttpHeaders &trailers);
    /// Complete the body and emit final callbacks.
    void handleBodyCompleted();
    /// Pump one block into the selected output stream off-loop.
    void writeSinkBlock(mem::ByteBlock data);
    /// Handle one sink write completion.
    void sinkWriteCompleted(std::exception_ptr failure);
    /// Open the collision-safe path sink on a bounded worker.
    void openPathSink();
    /// Continue path streaming after the temporary file was opened.
    void pathSinkOpened(stream::TempByteOutputStreamPtr temporary, std::exception_ptr failure);
    /// Flush, close, and atomically commit a completed path sink away from the event loop.
    void commitPathSink();
    /// Handle path-sink commit completion.
    void pathSinkCommitted(std::exception_ptr failure);
    /// Deliver one completed automatic aggregate.
    void deliverAutomatic(mem::ByteBlock data);
    /// Test the selected automatic Content-Type policy.
    [[nodiscard]] auto acceptsContentType(const HttpClientResponseHandler &handler) const -> bool;
    /// Test one media-type pattern.
    [[nodiscard]] static auto matchesContentTypePattern(const text::String &pattern, const HttpMediaType &mediaType)
        -> bool;
    /// Report a local response failure through the request.
    void fail(NetworkErrorReason reason, text::String description);
    /// Require a body awaiting policy selection.
    void verifyAwaitingPolicy() const;
    /// Emit response finalization exactly once.
    void finishFinal();

private:
    std::weak_ptr<HttpClientRequest> _request;                   ///< Owning request.
    std::unique_ptr<HttpClientResponseEventEditor> _eventEditor; ///< Stable public editor.
    HttpResponseHead _head;                                      ///< Immutable final head.
    HttpHeaders _trailers;                                       ///< Completed trailers.
    Url _effectiveUrl;                                           ///< Final effective request URL.
    unit::ItemCount _redirectCount;                              ///< Followed redirect count.
    Http1TransactionPtr _transaction;                            ///< Active internal transaction.
    stream::ByteOutputStreamPtr _output;                         ///< Optional sink.
    stream::TempByteOutputStreamPtr _temporaryOutput;            ///< Atomic path-download file.
    std::optional<path::Path> _destination;                      ///< Atomic path destination.
    std::shared_ptr<std::atomic_bool> _pathSinkCancelled;        ///< Cancellation observed by the commit worker.
    std::optional<mem::ByteBlock> _pendingSinkBlock;             ///< One outstanding sink block.
    std::optional<mem::ByteBlock> _sinkRemainder;                ///< Unwritten tail split at 16 KiB.
    std::optional<HttpClientResponseHandler> _automaticHandler;  ///< Automatic callback and policy.
    AggregateMode _aggregateMode{AggregateMode::None};           ///< Selected conversion.
    NetworkDataFn _onBodyData;                                   ///< Stream handler.
    NetworkDataFn _onBody;                                       ///< Byte aggregate handler.
    std::function<void(text::String)> _onText;                   ///< Text aggregate handler.
    std::function<void(text::json::JsonValue)> _onJson;          ///< JSON aggregate handler.
    std::function<void(const HttpHeaders &)> _onTrailers;        ///< Trailer handler.
    NetworkEventFn _onBodyCompleted;                             ///< Completion handler.
    HttpClientBodyProgressFn _onBodyProgress;                    ///< Successfully written progress.
    NetworkEventFn _onFinal;                                     ///< Final handler.
    bool _hasBody{};                                             ///< Semantic body presence.
    bool _policySelected{};                                      ///< Body policy guard.
    bool _sinkPending{};                                         ///< Worker operation guard.
    bool _inputCompleted{};                                      ///< Transaction input completed.
    bool _final{};                                               ///< Finalization guard.
    unit::ByteLength _committedLength;                           ///< Successfully written sink bytes.
    std::optional<unit::ByteLength> _expectedLength;             ///< Content-Length when known.
};

}
