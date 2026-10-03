// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsAlpnProtocol.hpp"

#include "../../../text/impl/UnsafeU8StringAccess.hpp"

#include <algorithm>
#include <cstring>

namespace erbsland::network::impl::tls_alpn_protocol {

using namespace text;

auto bytes(const String &protocol) noexcept -> mem::ConstByteSpan {
    const auto data = text::impl::UnsafeU8StringAccess{protocol}.dataSpan();
    return mem::toConstByteSpan(data);
}

auto equal(const String &left, const String &right) noexcept -> bool {
    const auto leftBytes = bytes(left);
    const auto rightBytes = bytes(right);
    return leftBytes.size() == rightBytes.size() &&
        (leftBytes.empty() || std::memcmp(leftBytes.data(), rightBytes.data(), leftBytes.size()) == 0);
}

auto less(const String &left, const String &right) noexcept -> bool {
    const auto leftBytes = bytes(left);
    const auto rightBytes = bytes(right);
    return std::lexicographical_compare(leftBytes.begin(), leftBytes.end(), rightBytes.begin(), rightBytes.end());
}

}
