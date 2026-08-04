// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpConnectionRequest.hpp"

#include "TcpConnectionDevice.hpp"

namespace erbsland::network::impl {

TcpConnectionRequest::TcpConnectionRequest(
    TcpAcceptedSocketPtr socket, network::ConnectionQuotaLease quotaLease, std::function<void()> releaseSlot) :
    _socket{std::move(socket)},
    _quotaLease{std::move(quotaLease)},
    _remoteEndpoint{_socket->remoteEndpoint()},
    _releaseSlot{std::move(releaseSlot)} {
}

TcpConnectionRequest::TcpConnectionRequest(TcpAcceptedSocketPtr socket, std::function<void()> releaseSlot) :
    TcpConnectionRequest{std::move(socket), network::ConnectionQuotaLease{}, std::move(releaseSlot)} {
}

TcpConnectionRequest::~TcpConnectionRequest() {
    reject();
}

auto TcpConnectionRequest::remoteEndpoint() const noexcept -> const IpEndpoint & {
    return _remoteEndpoint;
}

auto TcpConnectionRequest::state() const noexcept -> TcpConnectionRequestState {
    return _state.load();
}

void TcpConnectionRequest::reject() noexcept {
    auto expected = TcpConnectionRequestState::Pending;
    if (!_state.compare_exchange_strong(expected, TcpConnectionRequestState::Rejected)) {
        return;
    }
    auto quotaLease = network::ConnectionQuotaLease{};
    {
        const auto lock = std::scoped_lock{_mutex};
        _socket.reset();
        quotaLease = std::move(_quotaLease);
    }
    quotaLease.release();
    releaseSlot();
}

auto TcpConnectionRequest::isCompatibleWith(const TcpConnectionDevice &device) const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _state.load() == TcpConnectionRequestState::Pending && _socket != nullptr && device.canAccept(*_socket);
}

auto TcpConnectionRequest::claim() noexcept -> std::optional<Claim> {
    auto expected = TcpConnectionRequestState::Pending;
    if (!_state.compare_exchange_strong(expected, TcpConnectionRequestState::Accepted)) {
        return std::nullopt;
    }
    auto result = Claim{};
    {
        const auto lock = std::scoped_lock{_mutex};
        result.socket = std::move(_socket);
        result.quotaLease = std::move(_quotaLease);
    }
    releaseSlot();
    return result;
}

void TcpConnectionRequest::releaseSlot() noexcept {
    auto callback = std::function<void()>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        callback = std::move(_releaseSlot);
    }
    if (callback) {
        try {
            callback();
        } catch (...) {}
    }
}

}
