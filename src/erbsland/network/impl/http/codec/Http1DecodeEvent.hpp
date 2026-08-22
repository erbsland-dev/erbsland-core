// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1BodyFraming.hpp"
#include "Http1DecodeEvent_fwd.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../unit/ByteLength.hpp"
#include "../../../http/HttpRequestHead.hpp"
#include "../../../http/HttpResponseHead.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace erbsland::network::impl {

/// One polled output event from an HTTP/1.x decoder.
/// @tested{Http1CodecTest}
class Http1DecodeEvent final {
public:
    /// The event payload kind.
    enum class Kind : std::uint8_t {
        RequestHead,  ///< A complete request head is available.
        ResponseHead, ///< A complete response head is available.
        Body,         ///< A decoded body fragment is available.
        Trailers,     ///< A complete separate trailer section is available.
        Complete,     ///< The current message is complete.
    };

public: // factories
    /// Create a request-head event.
    [[nodiscard]] static auto requestHead(
        HttpRequestHead head, Http1BodyFraming framing, std::optional<unit::ByteLength> contentLength = {})
        -> Http1DecodeEvent;
    /// Create a response-head event.
    [[nodiscard]] static auto responseHead(
        HttpResponseHead head, Http1BodyFraming framing, std::optional<unit::ByteLength> contentLength = {})
        -> Http1DecodeEvent;
    /// Create a body event.
    [[nodiscard]] static auto body(mem::ByteBlock data) -> Http1DecodeEvent;
    /// Create a trailer event.
    [[nodiscard]] static auto trailers(HttpHeaders fields) -> Http1DecodeEvent;
    /// Create a completion event.
    [[nodiscard]] static auto complete() noexcept -> Http1DecodeEvent;

public: // accessors
    /// Get the event kind.
    [[nodiscard]] auto kind() const noexcept -> Kind { return _kind; }
    /// Get the request head, or an invalid placeholder for another kind.
    [[nodiscard]] auto request() const noexcept -> const HttpRequestHead & { return _request; }
    /// Get the response head, or an invalid placeholder for another kind.
    [[nodiscard]] auto response() const noexcept -> const HttpResponseHead & { return _response; }
    /// Get the body fragment, or an empty block for another kind.
    [[nodiscard]] auto data() const noexcept -> const mem::ByteBlock & { return _data; }
    /// Get the trailer fields, or an empty collection for another kind.
    [[nodiscard]] auto trailerFields() const noexcept -> const HttpHeaders & { return _trailers; }
    /// Get the selected body framing.
    [[nodiscard]] auto framing() const noexcept -> Http1BodyFraming { return _framing; }
    /// Get the selected fixed body length, if any.
    [[nodiscard]] auto contentLength() const noexcept -> const std::optional<unit::ByteLength> & {
        return _contentLength;
    }

private:
    Kind _kind{Kind::Complete};                        ///< Event payload kind.
    HttpRequestHead _request;                          ///< Optional request head.
    HttpResponseHead _response;                        ///< Optional response head.
    mem::ByteBlock _data;                              ///< Optional body fragment.
    HttpHeaders _trailers;                             ///< Optional trailer section.
    Http1BodyFraming _framing{Http1BodyFraming::None}; ///< Selected framing.
    std::optional<unit::ByteLength> _contentLength;    ///< Optional fixed length.
};

}
