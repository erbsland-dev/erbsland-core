// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../http_client/HttpClientRequestFn.hpp"
#include "../../../http_client/HttpClientResponseOptions.hpp"

#include <cstdint>
#include <utility>
#include <variant>

namespace erbsland::network::impl {

/// One type-safe automatic or low-level HTTP client response handler.
/// @tested{HttpClientTest}
class HttpClientResponseHandler final {
public:
    /// Response delivery category.
    enum class Kind : std::uint8_t {
        Bytes, ///< Bounded byte aggregation.
        Text,  ///< Bounded strict UTF-8 aggregation.
        Json,  ///< Bounded JSON aggregation.
        Head,  ///< Low-level response-head checkpoint.
    };
    /// Type-safe handler alternatives.
    using Callback = std::variant<
        std::monostate,
        HttpClientResponseFn,
        HttpClientTextResponseFn,
        HttpClientJsonResponseFn,
        HttpClientResponseHeadFn>;

public:
    /// Create default byte aggregation without an observer.
    HttpClientResponseHandler() = default;
    /// Create byte aggregation.
    HttpClientResponseHandler(HttpClientResponseFn callback, HttpClientResponseOptions options) :
        _kind{Kind::Bytes}, _options{std::move(options)}, _callback{std::move(callback)} {}
    /// Create strict UTF-8 aggregation.
    HttpClientResponseHandler(HttpClientTextResponseFn callback, HttpClientResponseOptions options) :
        _kind{Kind::Text}, _options{std::move(options)}, _callback{std::move(callback)} {}
    /// Create JSON aggregation.
    HttpClientResponseHandler(HttpClientJsonResponseFn callback, HttpClientResponseOptions options) :
        _kind{Kind::Json}, _options{std::move(options)}, _callback{std::move(callback)} {}
    /// Create low-level head handling.
    explicit HttpClientResponseHandler(HttpClientResponseHeadFn callback) :
        _kind{Kind::Head}, _callback{std::move(callback)} {}

public:
    /// Test whether an application callback is configured.
    [[nodiscard]] auto hasCallback() const noexcept -> bool { return _callback.index() != 0U; }
    /// Get the response delivery category.
    [[nodiscard]] auto kind() const noexcept -> Kind { return _kind; }
    /// Get automatic response options.
    [[nodiscard]] auto options() const noexcept -> const HttpClientResponseOptions & { return _options; }
    /// Get the type-safe callback.
    [[nodiscard]] auto callback() const noexcept -> const Callback & { return _callback; }

private:
    Kind _kind{Kind::Bytes};            ///< Delivery category.
    HttpClientResponseOptions _options; ///< Automatic aggregation policy.
    Callback _callback;                 ///< Optional application callback.
};

}
