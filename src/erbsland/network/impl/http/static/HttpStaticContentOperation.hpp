// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpStaticContentUse.hpp"

#include "../server/HttpServer_fwd.hpp"
#include "../server/HttpServerRequest_fwd.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../path/Path.hpp"
#include "../../../../stream/ByteInputStream_fwd.hpp"
#include "../../../../text/String.hpp"
#include "../../../../unit/ByteLength.hpp"
#include "../../../http/HttpStatus.hpp"
#include "../../../http_server/HttpStaticContent_fwd.hpp"
#include "../../../http_server/HttpStaticContentHandler_fwd.hpp"

#include <exception>
#include <memory>
#include <optional>
#include <vector>

namespace erbsland::network::impl {

/// One cancellable polymorphic static-content lookup and bounded response pump.
/// @notest{Internal implementation covered by HttpServerLiveTest.}
class HttpStaticContentOperation final : public std::enable_shared_from_this<HttpStaticContentOperation> {
private:
    /// One exact path probe in handler and index order.
    struct Candidate final {
        HttpStaticContentHandlerPtr handler; ///< Original user or built-in handler.
        path::Path path;                     ///< Validated decoded-NFC relative path.
    };

public:
    /// Retain the server, request, ordered handlers, and their active-use lease.
    HttpStaticContentOperation(
        std::shared_ptr<HttpServer> server,
        std::shared_ptr<HttpServerRequest> request,
        std::vector<HttpStaticContentHandlerPtr> handlers,
        HttpStaticContentUsePtr contentUse);
    /// Release this operation and all retained reservations.
    ~HttpStaticContentOperation();

public:
    /// Start asynchronous probing.
    void start();
    /// Resume a retained output block.
    void handleWritable();
    /// Cancel stale work and release quotas.
    void cancel() noexcept;

private:
    /// Build exact candidates for matching URL prefixes and trailing-slash indexes.
    void buildCandidates(const std::vector<HttpStaticContentHandlerPtr> &handlers);
    /// Parse one decoded URL prefix into complete segments.
    [[nodiscard]] static auto prefixSegments(const text::String &prefix) -> std::vector<text::String>;
    /// Test one decoded suffix segment against portable safety rules.
    [[nodiscard]] static auto isSafeSegment(const text::String &segment) noexcept -> bool;
    /// Build one relative path from decoded suffix segments.
    [[nodiscard]] static auto relativePath(const std::vector<text::String> &segments) -> path::Path;
    /// Continue with the next handler/path probe.
    void probeNext();
    /// Handle one probe completion.
    void probeCompleted(bool found, std::exception_ptr failure);
    /// Obtain generic content after an authoritative positive probe.
    void obtainContent();
    /// Handle generic content creation.
    void contentObtained(HttpStaticContentPtr content, std::exception_ptr failure);
    /// Open the selected content stream for GET.
    void openContent();
    /// Handle generic content opening.
    void contentOpened(stream::ByteInputStreamPtr stream, std::exception_ptr failure);
    /// Start the response after all failure-prone pre-commit work.
    void startResponse();
    /// Produce the next bounded stream block.
    void pump();
    /// Handle one stream read completion.
    void streamRead(std::optional<mem::ByteBlock> data, std::exception_ptr failure);
    /// Submit the retained block and advance only after acceptance.
    void submitBlock();
    /// Finish a successful response.
    void finish();
    /// Send one bounded framework error.
    void sendError(HttpStatus status);
    /// Report a post-commit source failure and abort the transaction.
    void sourceFailed();

private:
    std::weak_ptr<HttpServer> _server;           ///< Owning server.
    std::weak_ptr<HttpServerRequest> _request;   ///< Active request.
    HttpStaticContentUsePtr _contentUse;         ///< Keeps handler configuration frozen across workers.
    std::vector<Candidate> _candidates;          ///< Ordered exact probes.
    std::size_t _candidateIndex{};               ///< Next probe index.
    Candidate *_selected{};                      ///< Authoritative positive candidate.
    HttpStaticContentPtr _content;               ///< Generic selected representation.
    stream::ByteInputStreamPtr _stream;          ///< One-shot opened stream for GET.
    std::optional<mem::ByteBlock> _pendingBlock; ///< Retained rejected output block.
    unit::ByteLength _offset;                    ///< Bytes accepted by HTTP output.
    unit::ByteLength _length;                    ///< Exact declared representation length.
    unit::ByteLength _reservedMemoryLength;      ///< Aggregate retained-content reservation.
    unit::ByteLength _reservedQueueLength;       ///< Current output queue reservation.
    bool _activeReservation{};                   ///< Active-response quota reservation.
    bool _workerPending{};                       ///< One outstanding worker operation.
    bool _cancelled{};                           ///< Cancellation/generation guard.
};

using HttpStaticContentOperationPtr = std::shared_ptr<HttpStaticContentOperation>;

}
