// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpServerSession.hpp"

#include "HttpServerSessionEventEditor.hpp"

#include <utility>

namespace erbsland::network::impl {

HttpServerSession::HttpServerSession(
    event::EventsPtr ownerEvents,
    std::optional<text::String> identifier,
    HttpSessionDataPtr data,
    InvalidatedFn invalidatedFn) :
    network::HttpServerSession{std::move(ownerEvents)},
    _identifier{std::move(identifier)},
    _data{std::move(data)},
    _invalidatedFn{std::move(invalidatedFn)} {
}

HttpServerSession::~HttpServerSession() = default;

auto HttpServerSession::identifier() const -> std::optional<text::String> {
    return _identifier;
}

auto HttpServerSession::data() const noexcept -> const HttpSessionDataPtr & {
    return _data;
}

void HttpServerSession::setData(HttpSessionDataPtr data) {
    verifyCurrentOwnerEvents();
    _data = std::move(data);
}

auto HttpServerSession::isValid() const noexcept -> bool {
    return _valid;
}

void HttpServerSession::invalidate() {
    verifyCurrentOwnerEvents();
    if (!_valid) {
        return;
    }
    _valid = false;
    const auto self = std::static_pointer_cast<network::HttpServerSession>(shared_from_this());
    if (_invalidatedFn) {
        _invalidatedFn(self);
    }
    if (_onInvalidated) {
        _onInvalidated();
    }
    if (!_finalEmitted) {
        _finalEmitted = true;
        if (_onFinal) {
            _onFinal();
        }
    }
}

auto HttpServerSession::events() -> network::HttpServerSessionEventEditor & {
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<HttpServerSessionEventEditor>(shared_from_this(), std::move(target), *this);
    }
    return *_eventEditor;
}

}
