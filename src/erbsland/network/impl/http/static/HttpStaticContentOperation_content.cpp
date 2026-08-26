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
    auto contentUse = _contentUse;
    try {
        ResolverService::submit([weakSelf, weakServer, ownerEvents, handler, path, contentUse]() mutable -> void {
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
            contentUse.reset();
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
    auto contentUse = _contentUse;
    try {
        ResolverService::submit([weakSelf, weakServer, ownerEvents, content, contentUse]() mutable -> void {
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
            contentUse.reset();
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
    auto contentUse = _contentUse;
    try {
        ResolverService::submit(
            [weakSelf, weakServer, ownerEvents, stream, requestedLength, contentUse]() mutable -> void {
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
                contentUse.reset();
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
