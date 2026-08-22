// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1TransferCoding.hpp"

#include "../../../http/HttpHeaders.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace erbsland::network::impl {

/// Shared String-based parsing for HTTP/1.x message framing.
/// @tested{Http1CodecTest}
class Http1FramingParser final {
public:
    /// Parse Content-Length and Transfer-Encoding fields in one pass.
    /// @throws err::ParseError if either field is malformed, overflows, or is ambiguous.
    explicit Http1FramingParser(const HttpHeaders &headers);

public: // accessors
    /// Get the parsed Content-Length value.
    [[nodiscard]] auto contentLength() const noexcept -> std::optional<std::uint64_t> { return _contentLength; }
    /// Get the parsed transfer coding classification.
    [[nodiscard]] auto transferCoding() const noexcept -> Http1TransferCoding { return _transferCoding; }

public: // chunk metadata
    /// Parse a hexadecimal chunk size and validate any discarded extensions.
    /// @throws err::ParseError if the chunk metadata is malformed or overflows.
    [[nodiscard]] static auto chunkSize(const text::String &line) -> std::uint64_t;

private:
    /// Parse one Content-Length field value into the aggregate result.
    void parseContentLength(const text::String &text);
    /// Parse one Transfer-Encoding field value into the aggregate result.
    void parseTransferEncoding(const text::String &text);

private:
    std::optional<std::uint64_t> _contentLength;                    ///< Agreed Content-Length value.
    Http1TransferCoding _transferCoding{Http1TransferCoding::None}; ///< Parsed coding classification.
    std::size_t _codingCount{};                                     ///< Number of transfer codings.
};

}
