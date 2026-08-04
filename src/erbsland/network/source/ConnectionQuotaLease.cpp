// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConnectionQuotaLease.hpp"

#include "ConnectionQuota.hpp"

#include <utility>

namespace erbsland::network {

ConnectionQuotaLease::ConnectionQuotaLease(ConnectionQuotaPtr quota, std::optional<IpEndpoint> remoteEndpoint) noexcept
    :
    _quota{std::move(quota)}, _remoteEndpoint{std::move(remoteEndpoint)} {
}

ConnectionQuotaLease::~ConnectionQuotaLease() {
    release();
}

ConnectionQuotaLease::ConnectionQuotaLease(ConnectionQuotaLease &&other) noexcept :
    _quota{std::exchange(other._quota, {})}, _remoteEndpoint{std::move(other._remoteEndpoint)} {
    other._remoteEndpoint.reset();
}

auto ConnectionQuotaLease::operator=(ConnectionQuotaLease &&other) noexcept -> ConnectionQuotaLease & {
    if (this != &other) {
        release();
        _quota = std::exchange(other._quota, {});
        _remoteEndpoint = std::move(other._remoteEndpoint);
        other._remoteEndpoint.reset();
    }
    return *this;
}

void ConnectionQuotaLease::release() noexcept {
    auto quota = std::exchange(_quota, {});
    _remoteEndpoint.reset();
    if (quota != nullptr) {
        quota->release();
    }
}

}
