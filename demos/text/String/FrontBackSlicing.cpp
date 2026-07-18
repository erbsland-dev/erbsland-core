// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Side-based slicing takes a prefix or suffix without spelling out a full
/// range.
///
/// Use `slice(StringSide::Front, ByteLength)` when a previous byte-index search
/// already told you how long the prefix is. Use
/// `slice(StringSide::Back, CpLength)` when the suffix is naturally counted in
/// decoded characters. The byte length of that suffix can then be used to take
/// the remaining prefix efficiently.
void frontBackSlicing() {
    const auto route = el::String{"Rutt: Åsleden -> Nordljus"_el};

    // A byte index from `find()` can become the prefix byte length.
    const auto separator = route.find(" -> "_el);
    const auto origin = route.slice(el::StringSide::Front, separator.distanceFromZero());

    // A destination name is user-visible text, so take it as code points.
    const auto destination = route.slice(el::StringSide::Back, el::CpLength{8U});
    const auto withoutDestination = route.slice(el::StringSide::Front, route.length() - destination.length());

    el::io::printLine("Route: "_el, route);
    el::io::printLine("Origin: "_el, origin);
    el::io::printLine("Destination: "_el, destination);
    el::io::printLine("Without destination: ["_el, withoutDestination, "]"_el);
}

}
