// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConnectionQuota.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

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
    auto capacityChanged = false;
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_current >= _maximum.toSizeT()) {
            return std::nullopt;
        }
        ++_current;
        if (_current == _maximum.toSizeT()) {
            capacityChanged = true;
        }
    }
    if (capacityChanged) {
        _capacityChangedCallbacks.notify([](std::exception_ptr) noexcept -> void {});
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
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_current == 0U) {
            return;
        }
        --_current;
    }
    _capacityChangedCallbacks.notify([](std::exception_ptr) noexcept -> void {});
}

auto ConnectionQuota::addCapacityChanged(CapacityCallback callback) -> event::EventSubscription {
    return _capacityChangedCallbacks.add(std::move(callback));
}

}
