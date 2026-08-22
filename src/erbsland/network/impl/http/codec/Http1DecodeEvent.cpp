// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Http1DecodeEvent.hpp"

namespace erbsland::network::impl {

auto Http1DecodeEvent::requestHead(
    HttpRequestHead head, const Http1BodyFraming framing, std::optional<unit::ByteLength> contentLength)
    -> Http1DecodeEvent {
    auto result = Http1DecodeEvent{};
    result._kind = Kind::RequestHead;
    result._request = std::move(head);
    result._framing = framing;
    result._contentLength = contentLength;
    return result;
}

auto Http1DecodeEvent::responseHead(
    HttpResponseHead head, const Http1BodyFraming framing, std::optional<unit::ByteLength> contentLength)
    -> Http1DecodeEvent {
    auto result = Http1DecodeEvent{};
    result._kind = Kind::ResponseHead;
    result._response = std::move(head);
    result._framing = framing;
    result._contentLength = contentLength;
    return result;
}

auto Http1DecodeEvent::body(mem::ByteBlock data) -> Http1DecodeEvent {
    auto result = Http1DecodeEvent{};
    result._kind = Kind::Body;
    result._data = std::move(data);
    return result;
}

auto Http1DecodeEvent::trailers(HttpHeaders fields) -> Http1DecodeEvent {
    auto result = Http1DecodeEvent{};
    result._kind = Kind::Trailers;
    result._trailers = std::move(fields);
    return result;
}

auto Http1DecodeEvent::complete() noexcept -> Http1DecodeEvent {
    return {};
}

}
