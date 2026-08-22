// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpRouteHandler.hpp"

namespace erbsland::network::impl {

HttpRouteHandler::HttpRouteHandler(HttpServerRequestFn value, HttpServerRouteOptions routeOptions) :
    _kind{Kind::Bytes}, _options{std::move(routeOptions)}, _callback{std::move(value)} {
}

HttpRouteHandler::HttpRouteHandler(HttpServerTextRequestFn value, HttpServerRouteOptions routeOptions) :
    _kind{Kind::Text}, _options{std::move(routeOptions)}, _callback{std::move(value)} {
}

HttpRouteHandler::HttpRouteHandler(HttpServerJsonRequestFn value, HttpServerRouteOptions routeOptions) :
    _kind{Kind::Json}, _options{std::move(routeOptions)}, _callback{std::move(value)} {
}

HttpRouteHandler::HttpRouteHandler(HttpServerRequestHeadFn value) : _kind{Kind::Head}, _callback{std::move(value)} {
}

}
