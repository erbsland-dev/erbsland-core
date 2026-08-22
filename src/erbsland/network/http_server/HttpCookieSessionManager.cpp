// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpCookieSessionManager.hpp"

#include "../impl/http/cookie/HttpCookieSessionManagerData.hpp"

#include <utility>

namespace erbsland::network {

auto HttpCookieSessionManager::create(HttpCookieSessionManagerOptions options) -> HttpCookieSessionManagerPtr {
    return HttpCookieSessionManagerPtr{new HttpCookieSessionManager{std::move(options)}};
}

HttpCookieSessionManager::HttpCookieSessionManager(HttpCookieSessionManagerOptions options) :
    _data{std::make_shared<impl::HttpCookieSessionManagerData>(std::move(options))} {
}

auto HttpCookieSessionManager::selectSession(const HttpServerSessionContext &context) -> HttpServerSessionSelection {
    return _data->select(context);
}

auto HttpCookieSessionManager::sessionInvalidated(const HttpServerSessionPtr &session) -> HttpHeaders {
    return _data->invalidate(session);
}

}
