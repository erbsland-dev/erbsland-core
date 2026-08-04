// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConnectionQuota.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

#include <vector>

namespace erbsland::network {

using namespace text::literals;

auto ConnectionQuota::create(const unit::ItemCount maximum) -> ConnectionQuotaPtr {
    if (!maximum.isFinite() || maximum.isZero()) {
        throw err::ParameterError{"A connection quota maximum must be positive and finite."_el, "maximum"_el};
    }
    return ConnectionQuotaPtr{new ConnectionQuota{maximum}};
}

ConnectionQuota::ConnectionQuota(const unit::ItemCount maximum) noexcept : _maximum{maximum} {
}

auto ConnectionQuota::tryAcquire(std::optional<IpEndpoint> remoteEndpoint) -> std::optional<ConnectionQuotaLease> {
    auto callbacks = std::vector<CapacityCallback>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_current >= _maximum.toSizeT()) {
            return std::nullopt;
        }
        ++_current;
        if (_current == _maximum.toSizeT()) {
            callbacks.reserve(_subscribers.size());
            for (const auto &entry : _subscribers) {
                callbacks.push_back(entry.second);
            }
        }
    }
    for (const auto &callback : callbacks) {
        try {
            callback();
        } catch (...) {}
    }
    return ConnectionQuotaLease{shared_from_this(), std::move(remoteEndpoint)};
}

auto ConnectionQuota::current() const noexcept -> unit::ItemCount {
    const auto lock = std::scoped_lock{_mutex};
    return unit::ItemCount::fromSizeT(_current);
}

auto ConnectionQuota::available() const noexcept -> unit::ItemCount {
    const auto lock = std::scoped_lock{_mutex};
    return unit::ItemCount::fromSizeT(_maximum.toSizeT() - _current);
}

void ConnectionQuota::release() noexcept {
    auto callbacks = std::vector<CapacityCallback>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_current == 0U) {
            return;
        }
        --_current;
        callbacks.reserve(_subscribers.size());
        for (const auto &entry : _subscribers) {
            callbacks.push_back(entry.second);
        }
    }
    for (const auto &callback : callbacks) {
        try {
            callback();
        } catch (...) {}
    }
}

auto ConnectionQuota::subscribe(CapacityCallback callback) -> std::uint64_t {
    const auto lock = std::scoped_lock{_mutex};
    const auto id = _nextSubscriptionId++;
    _subscribers.emplace(id, std::move(callback));
    return id;
}

void ConnectionQuota::unsubscribe(const std::uint64_t id) noexcept {
    const auto lock = std::scoped_lock{_mutex};
    _subscribers.erase(id);
}

}
