// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticContentOperation.hpp"

#include "../server/HttpServer.hpp"
#include "../server/HttpServerRequest.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../stream/ByteInputStream.hpp"
#include "../../../../stream/StreamReadStatus.hpp"
#include "../../../../text/CaseSensitivity.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../http/HttpHeaders.hpp"
#include "../../../http/HttpMethod.hpp"
#include "../../../http/HttpMethodType.hpp"
#include "../../../http/HttpResponseHead.hpp"
#include "../../../http_server/HttpMediaTypeMapping.hpp"
#include "../../../http_server/HttpStaticContent.hpp"
#include "../../../http_server/HttpStaticContentHandler.hpp"
#include "../../../source/Connection.hpp"
#include "../../../source/NetworkErrorContext.hpp"
#include "../../ResolverService.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpStaticContentOperation::HttpStaticContentOperation(
    std::shared_ptr<HttpServer> server,
    std::shared_ptr<HttpServerRequest> request,
    std::vector<HttpStaticContentHandlerPtr> handlers,
    HttpStaticContentUsePtr contentUse) :
    _server{std::move(server)}, _request{std::move(request)}, _contentUse{std::move(contentUse)} {
    buildCandidates(handlers);
}

HttpStaticContentOperation::~HttpStaticContentOperation() {
    cancel();
}

void HttpStaticContentOperation::start() {
    const auto server = _server.lock();
    const auto request = _request.lock();
    if (server == nullptr || request == nullptr) {
        cancel();
        return;
    }
    if (!server->reserveStaticResponse()) {
        sendError(HttpStatus::ServiceUnavailable);
        return;
    }
    _activeReservation = true;
    request->rejectBody();
    probeNext();
}

void HttpStaticContentOperation::handleWritable() {
    if (!_cancelled && _pendingBlock.has_value()) {
        submitBlock();
    }
}

void HttpStaticContentOperation::cancel() noexcept {
    if (_cancelled) {
        return;
    }
    _cancelled = true;
    if (const auto server = _server.lock()) {
        if (!_reservedQueueLength.isZero()) {
            server->releaseStaticQueue(_reservedQueueLength);
        }
        if (!_reservedMemoryLength.isZero()) {
            server->releaseStaticMemory(_reservedMemoryLength);
        }
        if (_activeReservation) {
            server->releaseStaticResponse();
        }
    }
    _reservedQueueLength = {};
    _reservedMemoryLength = {};
    _activeReservation = false;
    _pendingBlock.reset();
    auto stream = std::move(_stream);
    auto content = std::move(_content);
    _contentUse.reset();
    if (stream == nullptr && content == nullptr) {
        return;
    }
    try {
        ResolverService::submit([stream = std::move(stream), content = std::move(content)]() mutable -> void {
            if (stream != nullptr) {
                stream->abort();
            }
            content.reset();
        });
    } catch (...) {
        if (stream != nullptr) {
            stream->abort();
        }
    }
}

void HttpStaticContentOperation::buildCandidates(const std::vector<HttpStaticContentHandlerPtr> &handlers) {
    const auto request = _request.lock();
    if (request == nullptr) {
        return;
    }
    const auto &requestSegments = request->segments();
    const auto trailingSlash = request->path().endsWith("/"_el);
    for (const auto &handler : handlers) {
        const auto prefix = prefixSegments(handler->urlPrefix());
        if (requestSegments.size() < prefix.size()) {
            continue;
        }
        auto matches = true;
        for (std::size_t index = 0U; index < prefix.size(); ++index) {
            if (requestSegments[index] != prefix[index]) {
                matches = false;
                break;
            }
        }
        if (!matches) {
            continue;
        }
        auto suffix = std::vector<String>{
            requestSegments.begin() + static_cast<std::ptrdiff_t>(prefix.size()), requestSegments.end()};
        if (trailingSlash && !suffix.empty() && suffix.back().isEmpty()) {
            suffix.pop_back();
        }
        if (std::ranges::any_of(suffix, [](const String &segment) -> bool { return !isSafeSegment(segment); })) {
            continue;
        }
        if (trailingSlash) {
            for (const auto &indexName : handler->indexFileNames()) {
                auto indexed = suffix;
                indexed.emplace_back(indexName);
                auto path = relativePath(indexed);
                if (!path.isEmpty() && path.isValid() && path.isRelative()) {
                    _candidates.emplace_back(Candidate{handler, std::move(path)});
                }
            }
        } else if (!suffix.empty()) {
            auto path = relativePath(suffix);
            if (!path.isEmpty() && path.isValid() && path.isRelative()) {
                _candidates.emplace_back(Candidate{handler, std::move(path)});
            }
        }
    }
}

auto HttpStaticContentOperation::prefixSegments(const String &prefix) -> std::vector<String> {
    auto result = std::vector<String>{};
    auto reader = StringCharReader{prefix};
    reader.advanceIf(U'/');
    auto segment = StringEditor{};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'/') {
            result.emplace_back(String{segment});
            segment.clear();
        } else {
            segment.append(character);
        }
    }
    if (!segment.isEmpty()) {
        result.emplace_back(String{segment});
    }
    return result;
}

auto HttpStaticContentOperation::isSafeSegment(const String &segment) noexcept -> bool {
    if (!segment.isValidUtf8() || segment.isEmpty() || segment == "."_el || segment == ".."_el ||
        segment.endsWith("."_el) || segment.endsWith(" "_el) || segment.length() > unit::ByteLength{255U}) {
        return false;
    }
    auto reader = StringCharReader{segment};
    auto baseName = StringEditor{};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'\0' || character == U'/' || character == U'\\' || character == U':') {
            return false;
        }
        if (character == U'.') {
            break;
        }
        baseName.append(character);
    }
    const auto base = String{baseName};
    const auto compare = cCaseInsensitive.asciiComparisonFn();
    const auto isDevicePrefix = [&](const StringLiteral &prefix) -> bool {
        return base.length() == unit::ByteLength{4U} && base.startsWith(prefix, compare) &&
            base.charAt(StringSide::Back).isAsciiDigit() && base.charAt(StringSide::Back) != U'0';
    };
    return base.compare("con"_el, compare) != std::strong_ordering::equal &&
        base.compare("prn"_el, compare) != std::strong_ordering::equal &&
        base.compare("aux"_el, compare) != std::strong_ordering::equal &&
        base.compare("nul"_el, compare) != std::strong_ordering::equal && !isDevicePrefix("com"_el) &&
        !isDevicePrefix("lpt"_el);
}

auto HttpStaticContentOperation::relativePath(const std::vector<String> &segments) -> path::Path {
    auto result = StringEditor{};
    for (const auto &segment : segments) {
        if (!result.isEmpty()) {
            result.append(U'/');
        }
        result.append(segment);
    }
    return path::Path{String{result}};
}

void HttpStaticContentOperation::probeNext() {
    if (_cancelled) {
        return;
    }
    const auto server = _server.lock();
    if (server == nullptr) {
        cancel();
        return;
    }
    if (_candidateIndex >= _candidates.size()) {
        sendError(HttpStatus::NotFound);
        return;
    }
    _selected = &_candidates[_candidateIndex++];
    if (!server->reserveStaticOperation()) {
        sendError(HttpStatus::ServiceUnavailable);
        return;
    }
    _workerPending = true;
    const auto weakSelf = std::weak_ptr<HttpStaticContentOperation>{shared_from_this()};
    const auto weakServer = std::weak_ptr<HttpServer>{server};
    const auto ownerEvents = server->ownerEvents();
    const auto handler = _selected->handler;
    const auto path = _selected->path;
    const auto contentUse = _contentUse;
    try {
        ResolverService::submit([weakSelf, weakServer, ownerEvents, handler, path, contentUse]() -> void {
            auto found = false;
            auto failure = std::exception_ptr{};
            try {
                if (contentUse == nullptr) {
                    throw err::LogicError{"Static-content configuration is no longer frozen."_el};
                }
                found = handler->hasPath(path);
            } catch (...) {
                failure = std::current_exception();
            }
            ownerEvents->invoke([weakSelf, weakServer, found, failure]() -> void {
                if (const auto target = weakServer.lock()) {
                    target->releaseStaticOperation();
                }
                if (const auto self = weakSelf.lock()) {
                    self->probeCompleted(found, failure);
                }
            });
        });
    } catch (...) {
        _workerPending = false;
        server->releaseStaticOperation();
        sendError(HttpStatus::ServiceUnavailable);
    }
}

void HttpStaticContentOperation::probeCompleted(const bool found, const std::exception_ptr failure) {
    _workerPending = false;
    if (_cancelled) {
        return;
    }
    if (failure != nullptr) {
        sendError(HttpStatus::InternalServerError);
        return;
    }
    if (!found) {
        probeNext();
        return;
    }
    const auto request = _request.lock();
    if (request == nullptr) {
        cancel();
        return;
    }
    const auto method = request->head().method();
    if (method != HttpMethod{HttpMethodType::Get} && method != HttpMethod{HttpMethodType::Head}) {
        auto headers = HttpHeaders{};
        headers.setField(HttpFieldType::Allow, "GET, HEAD"_el);
        request->sendResponse(
            HttpResponseHead{
                request->head().version(),
                HttpStatus::MethodNotAllowed,
                HttpStatus{HttpStatus::MethodNotAllowed}.defaultReasonPhrase(),
                std::move(headers)},
            {});
        return;
    }
    obtainContent();
}

void HttpStaticContentOperation::obtainContent() {
    const auto server = _server.lock();
    if (_cancelled || server == nullptr || _selected == nullptr) {
        cancel();
        return;
    }
    if (!server->reserveStaticOperation()) {
        sendError(HttpStatus::ServiceUnavailable);
        return;
    }
    _workerPending = true;
    const auto weakSelf = std::weak_ptr<HttpStaticContentOperation>{shared_from_this()};
    const auto weakServer = std::weak_ptr<HttpServer>{server};
    const auto ownerEvents = server->ownerEvents();
    const auto handler = _selected->handler;
    const auto path = _selected->path;
    const auto contentUse = _contentUse;
    try {
        ResolverService::submit([weakSelf, weakServer, ownerEvents, handler, path, contentUse]() -> void {
            auto content = HttpStaticContentPtr{};
            auto failure = std::exception_ptr{};
            try {
                if (contentUse == nullptr) {
                    throw err::LogicError{"Static-content configuration is no longer frozen."_el};
                }
                content = handler->getContent(path);
            } catch (...) {
                failure = std::current_exception();
            }
            ownerEvents->invoke([weakSelf, weakServer, content = std::move(content), failure]() mutable -> void {
                if (const auto target = weakServer.lock()) {
                    target->releaseStaticOperation();
                }
                if (const auto self = weakSelf.lock()) {
                    self->contentObtained(std::move(content), failure);
                }
            });
        });
    } catch (...) {
        _workerPending = false;
        server->releaseStaticOperation();
        sendError(HttpStatus::ServiceUnavailable);
    }
}

void HttpStaticContentOperation::contentObtained(HttpStaticContentPtr content, const std::exception_ptr failure) {
    _workerPending = false;
    if (_cancelled) {
        return;
    }
    if (failure != nullptr || content == nullptr) {
        sendError(HttpStatus::InternalServerError);
        return;
    }
    const auto length = content->length();
    const auto retainedMemoryLength = content->retainedMemoryLength();
    if (length.isInfinite() || retainedMemoryLength.isInfinite()) {
        sendError(HttpStatus::InternalServerError);
        return;
    }
    const auto server = _server.lock();
    const auto request = _request.lock();
    if (server == nullptr || request == nullptr) {
        cancel();
        return;
    }
    _length = length;
    _reservedMemoryLength = retainedMemoryLength;
    if (!server->reserveStaticMemory(_reservedMemoryLength)) {
        _reservedMemoryLength = {};
        sendError(HttpStatus::ServiceUnavailable);
        return;
    }
    _content = std::move(content);
    if (request->head().method() == HttpMethod{HttpMethodType::Head}) {
        startResponse();
        return;
    }
    openContent();
}

void HttpStaticContentOperation::openContent() {
    const auto server = _server.lock();
    if (_cancelled || server == nullptr || _content == nullptr) {
        cancel();
        return;
    }
    if (!server->reserveStaticOperation()) {
        sendError(HttpStatus::ServiceUnavailable);
        return;
    }
    _workerPending = true;
    const auto weakSelf = std::weak_ptr<HttpStaticContentOperation>{shared_from_this()};
    const auto weakServer = std::weak_ptr<HttpServer>{server};
    const auto ownerEvents = server->ownerEvents();
    const auto content = _content;
    const auto contentUse = _contentUse;
    try {
        ResolverService::submit([weakSelf, weakServer, ownerEvents, content, contentUse]() -> void {
            auto stream = stream::ByteInputStreamPtr{};
            auto failure = std::exception_ptr{};
            try {
                if (contentUse == nullptr) {
                    throw err::LogicError{"Static-content configuration is no longer frozen."_el};
                }
                stream = content->open();
            } catch (...) {
                failure = std::current_exception();
            }
            ownerEvents->invoke([weakSelf, weakServer, stream = std::move(stream), failure]() mutable -> void {
                if (const auto target = weakServer.lock()) {
                    target->releaseStaticOperation();
                }
                if (const auto self = weakSelf.lock()) {
                    self->contentOpened(std::move(stream), failure);
                }
            });
        });
    } catch (...) {
        _workerPending = false;
        server->releaseStaticOperation();
        sendError(HttpStatus::ServiceUnavailable);
    }
}

void HttpStaticContentOperation::contentOpened(stream::ByteInputStreamPtr stream, const std::exception_ptr failure) {
    _workerPending = false;
    if (_cancelled) {
        return;
    }
    if (failure != nullptr || stream == nullptr) {
        sendError(HttpStatus::InternalServerError);
        return;
    }
    _stream = std::move(stream);
    startResponse();
}

void HttpStaticContentOperation::startResponse() {
    const auto request = _request.lock();
    if (_cancelled || request == nullptr || _selected == nullptr) {
        cancel();
        return;
    }
    auto headers = HttpHeaders{};
    headers.setField(HttpFieldType::ContentLength, String::fromInteger(_length.toRawValue()));
    headers.setField(
        HttpFieldType::ContentType,
        _selected->handler->mediaTypeMapping()->mediaType(_selected->path.toString()).toString());
    request->startResponse(
        HttpResponseHead{
            request->head().version(),
            HttpStatus::Ok,
            HttpStatus{HttpStatus::Ok}.defaultReasonPhrase(),
            std::move(headers)});
    if (request->head().method() == HttpMethod{HttpMethodType::Head} || _length.isZero()) {
        finish();
        return;
    }
    pump();
}

void HttpStaticContentOperation::pump() {
    if (_cancelled || _pendingBlock.has_value() || _workerPending) {
        return;
    }
    if (_offset >= _length) {
        finish();
        return;
    }
    const auto server = _server.lock();
    if (server == nullptr || _stream == nullptr) {
        sourceFailed();
        return;
    }
    const auto requestedLength = std::min(server->_options.staticContentChunkLength(), _length - _offset);
    if (!server->reserveStaticQueue(requestedLength)) {
        sourceFailed();
        return;
    }
    _reservedQueueLength = requestedLength;
    if (!server->reserveStaticOperation()) {
        server->releaseStaticQueue(_reservedQueueLength);
        _reservedQueueLength = {};
        sourceFailed();
        return;
    }
    _workerPending = true;
    const auto weakSelf = std::weak_ptr<HttpStaticContentOperation>{shared_from_this()};
    const auto weakServer = std::weak_ptr<HttpServer>{server};
    const auto ownerEvents = server->ownerEvents();
    const auto stream = _stream;
    const auto contentUse = _contentUse;
    try {
        ResolverService::submit([weakSelf, weakServer, ownerEvents, stream, requestedLength, contentUse]() -> void {
            auto data = std::optional<mem::ByteBlock>{};
            auto failure = std::exception_ptr{};
            try {
                if (contentUse == nullptr) {
                    throw err::LogicError{"Static-content configuration is no longer frozen."_el};
                }
                auto result = stream->read(requestedLength);
                if (result == stream::StreamReadStatus::Data) {
                    data = result.takeData();
                }
            } catch (...) {
                failure = std::current_exception();
            }
            ownerEvents->invoke([weakSelf, weakServer, data = std::move(data), failure]() mutable -> void {
                if (const auto target = weakServer.lock()) {
                    target->releaseStaticOperation();
                }
                if (const auto self = weakSelf.lock()) {
                    self->streamRead(std::move(data), failure);
                }
            });
        });
    } catch (...) {
        _workerPending = false;
        server->releaseStaticOperation();
        server->releaseStaticQueue(_reservedQueueLength);
        _reservedQueueLength = {};
        sourceFailed();
    }
}

void HttpStaticContentOperation::streamRead(std::optional<mem::ByteBlock> data, const std::exception_ptr failure) {
    _workerPending = false;
    if (_cancelled) {
        return;
    }
    if (failure != nullptr || !data.has_value() || data->isEmpty() || data->length() > _reservedQueueLength ||
        data->length() > _length - _offset) {
        sourceFailed();
        return;
    }
    if (data->length() < _reservedQueueLength) {
        if (const auto server = _server.lock()) {
            server->releaseStaticQueue(_reservedQueueLength - data->length());
        }
        _reservedQueueLength = data->length();
    }
    _pendingBlock = std::move(*data);
    submitBlock();
}

void HttpStaticContentOperation::submitBlock() {
    const auto request = _request.lock();
    const auto server = _server.lock();
    if (_cancelled || request == nullptr || server == nullptr || !_pendingBlock.has_value()) {
        cancel();
        return;
    }
    const auto status = request->sendBody(*_pendingBlock);
    if (status.wouldBlock()) {
        return;
    }
    if (status.isClosed()) {
        cancel();
        return;
    }
    _offset += _pendingBlock->length();
    _pendingBlock.reset();
    server->releaseStaticQueue(_reservedQueueLength);
    _reservedQueueLength = {};
    pump();
}

void HttpStaticContentOperation::finish() {
    if (const auto request = _request.lock(); request != nullptr && !request->isFinal()) {
        request->finishBody({});
    }
}

void HttpStaticContentOperation::sendError(const HttpStatus status) {
    if (const auto request = _request.lock(); request != nullptr && !request->isResponseStarted()) {
        request->sendError(status, {});
    } else {
        sourceFailed();
    }
}

void HttpStaticContentOperation::sourceFailed() {
    const auto request = _request.lock();
    if (const auto server = _server.lock(); server != nullptr && server->_onError) {
        auto context = NetworkErrorContext{
            "Static content failed"_el, "A static-content source failed after the response was committed."_el};
        context.setReason(NetworkErrorReason::ContentSourceFailed);
        server->_onError(context);
    }
    if (request != nullptr && request->connection() != nullptr) {
        request->connection()->abort();
    }
    cancel();
}

}
