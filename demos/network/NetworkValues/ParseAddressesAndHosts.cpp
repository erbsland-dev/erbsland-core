// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Use `IpAddress` when input must be numeric, `HostName` when it must be a name, and `Host` when either form is valid.
/// The non-throwing `fromString()` factories are suitable for user input, while `fromStringOrThrow()` keeps trusted
/// configuration and constants concise. Formatting always returns a canonical representation.
void parseAddressesAndHosts() {
    const auto address = el::IpAddress::fromStringOrThrow("2001:0DB8:0:0::42"_el);
    const auto hostName = el::HostName::fromStringOrThrow("stelling.example"_el);
    const auto numericHost = el::Host::fromStringOrThrow("192.0.2.42"_el);
    const auto namedHost = el::Host::fromStringOrThrow("stelling.example"_el);

    el::io::printLine("Canonical address: "_el, address);
    el::io::printLine("Host name: "_el, hostName);
    el::io::printLine("Numeric host contains an address: "_el, numericHost.isAddress());
    el::io::printLine("Named host contains a name: "_el, namedHost.isName());

    const auto invalidHost = el::Host::fromString("not a host"_el);
    el::io::printLine("Invalid user input accepted: "_el, invalidHost.has_value());
}

}
