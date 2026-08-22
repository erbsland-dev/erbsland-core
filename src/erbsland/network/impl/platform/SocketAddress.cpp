// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SocketAddress.hpp"

#include <cstdint>
#include <cstring>

namespace erbsland::network::impl {

auto SocketAddress::data() const noexcept -> const sockaddr * {
    return reinterpret_cast<const sockaddr *>(&_storage);
}

auto SocketAddress::data() noexcept -> sockaddr * {
    return reinterpret_cast<sockaddr *>(&_storage);
}

auto SocketAddress::family() const noexcept -> int {
    return data()->sa_family;
}

auto SocketAddress::toEndpoint() const noexcept -> std::optional<IpEndpoint> {
    auto bytes = IpAddress::Bytes{};
    if (family() == AF_INET) {
        const auto &address = reinterpret_cast<const sockaddr_in &>(_storage);
        const auto source = reinterpret_cast<const std::uint8_t *>(&address.sin_addr);
        for (auto index = std::size_t{0}; index < 4U; ++index) {
            bytes.set(unit::ByteIndex::fromSizeT(index), mem::Byte{source[index]});
        }
        return IpEndpoint{IpAddress::fromBytes(IpVersion::V4, bytes), Port{ntohs(address.sin_port)}};
    }
    if (family() == AF_INET6) {
        const auto &address = reinterpret_cast<const sockaddr_in6 &>(_storage);
        const auto source = reinterpret_cast<const std::uint8_t *>(&address.sin6_addr);
        for (auto index = std::size_t{0}; index < 16U; ++index) {
            bytes.set(unit::ByteIndex::fromSizeT(index), mem::Byte{source[index]});
        }
        return IpEndpoint{
            IpAddress::fromBytes(IpVersion::V6, bytes), Port{ntohs(address.sin6_port)}, ScopeId{address.sin6_scope_id}};
    }
    return std::nullopt;
}

auto SocketAddress::fromEndpoint(const IpEndpoint &endpoint) noexcept -> SocketAddress {
    auto result = SocketAddress{};
    const auto bytes = endpoint.address().bytes().span();
    if (endpoint.address().isV4()) {
        auto &address = reinterpret_cast<sockaddr_in &>(result._storage);
        address.sin_family = AF_INET;
        address.sin_port = htons(endpoint.port().toRawValue());
        std::memcpy(&address.sin_addr, bytes.data(), 4U);
        result._size = static_cast<int>(sizeof(address));
        return result;
    }
    auto &address = reinterpret_cast<sockaddr_in6 &>(result._storage);
    address.sin6_family = AF_INET6;
    address.sin6_port = htons(endpoint.port().toRawValue());
    address.sin6_scope_id = endpoint.scopeId().toRawValue();
    std::memcpy(&address.sin6_addr, bytes.data(), 16U);
    result._size = static_cast<int>(sizeof(address));
    return result;
}

}
