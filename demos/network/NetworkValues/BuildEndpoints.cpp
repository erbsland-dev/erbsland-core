// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Pair an `IpAddress` with a `Port` in `IpEndpoint` after resolution, or use `HostEndpoint` while a host name is still
/// allowed. IPv6 endpoints use brackets, and link-local IPv6 addresses can carry a numeric `ScopeId`.
void buildEndpoints() {
    const auto service = el::HostEndpoint::fromStringOrThrow("stelling.example:443"_el);
    const auto resolved = el::IpEndpoint::fromStringOrThrow("192.0.2.42:443"_el);
    const auto linkLocal = el::IpEndpoint::fromStringOrThrow("[fe80::42%7]:443"_el);

    el::io::printLine("Named endpoint: "_el, service.toString());
    el::io::printLine("Resolved endpoint: "_el, resolved.toString());
    el::io::printLine("Scoped IPv6 endpoint: "_el, linkLocal.toString());
    el::io::printLine("IPv6 scope identifier: "_el, linkLocal.scopeId().toRawValue());
}

}
