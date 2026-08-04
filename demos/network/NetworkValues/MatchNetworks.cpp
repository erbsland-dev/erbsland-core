// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `IpNetwork` represents a normalized IPv4 or IPv6 CIDR range.
/// Use `contains()` for allowlists, routing decisions, or other address-range checks.
void matchNetworks() {
    const auto network = el::IpNetwork::fromStringOrThrow("192.0.2.129/24"_el);
    const auto inside = el::IpAddress::fromStringOrThrow("192.0.2.42"_el);
    const auto outside = el::IpAddress::fromStringOrThrow("198.51.100.42"_el);

    el::io::printLine("Normalized network: "_el, network);
    el::io::printLine(inside, " belongs to the network: "_el, network.contains(inside));
    el::io::printLine(outside, " belongs to the network: "_el, network.contains(outside));
    el::io::printLine("Address range: "_el, network.firstAddress(), " - "_el, network.lastAddress());
}

}
