// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../http_server/HttpServerRequestFn.hpp"
#include "../../../http_server/HttpServerRouteOptions.hpp"

#include <cstdint>
#include <utility>
#include <variant>

namespace erbsland::network::impl {

/// One validated type-safe route callback and its automatic body options.
/// @tested{HttpRoutesTest HttpServerLiveTest}
class HttpRouteHandler final {
public:
    /// Internal HTTP route handler category.
    enum class Kind : uint8_t {
        Bytes,
        Text,
        Json,
        Head,
    };

    /// Type-safe callback variant for every route family.
    using Callback = std::variant<
        std::monostate,
        HttpServerRequestFn,
        HttpServerTextRequestFn,
        HttpServerJsonRequestFn,
        HttpServerRequestHeadFn>;

public: // defaults
    HttpRouteHandler() = default;
    ~HttpRouteHandler() = default;
    HttpRouteHandler(const HttpRouteHandler &) = default;
    HttpRouteHandler(HttpRouteHandler &&) noexcept = default;
    auto operator=(const HttpRouteHandler &) -> HttpRouteHandler & = default;
    auto operator=(HttpRouteHandler &&) noexcept -> HttpRouteHandler & = default;

public:
    /// Create an aggregated byte handler.
    HttpRouteHandler(HttpServerRequestFn value, HttpServerRouteOptions routeOptions);
    /// Create an aggregated strict UTF-8 handler.
    HttpRouteHandler(HttpServerTextRequestFn value, HttpServerRouteOptions routeOptions);
    /// Create an aggregated JSON handler.
    HttpRouteHandler(HttpServerJsonRequestFn value, HttpServerRouteOptions routeOptions);
    /// Create a low-level request-head handler.
    explicit HttpRouteHandler(HttpServerRequestHeadFn value);

public:
    /// Test whether this value contains an application callback.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _callback.index() != 0U; }
    /// Get the route handler category.
    [[nodiscard]] auto kind() const noexcept -> Kind { return _kind; }
    /// Get automatic body options.
    [[nodiscard]] auto options() const noexcept -> const HttpServerRouteOptions & { return _options; }
    /// Get the type-safe application callback.
    [[nodiscard]] auto callback() const noexcept -> const Callback & { return _callback; }

private:
    Kind _kind{Kind::Head};          ///< Dispatch and body conversion kind.
    HttpServerRouteOptions _options; ///< Automatic-body policy.
    Callback _callback;              ///< Type-safe application callback.
};

}
