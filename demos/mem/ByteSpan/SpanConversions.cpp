// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// View standard byte storage through the Erbsland byte-span types.
///
/// `toByteSpan()` and `toConstByteSpan()` adapt spans of `std::byte`,
/// `uint8_t`, and `char` without copying their data. A writable view changes
/// the original storage; a const view provides read-only access to it.
void spanConversions() {
    auto filters = std::array{std::byte{1U}, std::byte{2U}, std::byte{3U}};
    auto intensities = std::array<uint8_t, 3>{31U, 47U, 63U};
    auto catalogId = std::array{'M', '4', '?'};

    // Adapt mutable standard storage and edit it through a byte view.
    auto filterBytes = el::toByteSpan(std::span{filters});
    auto intensityBytes = el::toByteSpan(std::span{intensities});
    auto catalogBytes = el::toByteSpan(std::span{catalogId});
    filterBytes.front() = el::Byte{7U};
    intensityBytes.back() = el::Byte{64U};
    catalogBytes.back() = el::Byte::fromChar('2');

    // Convert const spans when an operation only needs to inspect the bytes.
    const auto &constFilters = filters;
    const auto &constIntensities = intensities;
    const auto &constCatalogId = catalogId;
    const auto readOnlyFilters = el::toConstByteSpan(std::span{constFilters});
    const auto readOnlyIntensities = el::toConstByteSpan(std::span{constIntensities});
    const auto readOnlyCatalog = el::toConstByteSpan(std::span{constCatalogId});

    el::io::printLine("Observation        : Νεφέλωμα του Ωρίωνα"_el);
    el::io::printLine("First filter       : "_el, readOnlyFilters.front().toUInt32());
    el::io::printLine("Last intensity     : "_el, readOnlyIntensities.back().toUInt32());
    el::io::printLine(
        "Catalog ID         : "_el,
        readOnlyCatalog[0].toChar(),
        readOnlyCatalog[1].toChar(),
        readOnlyCatalog[2].toChar());
}

}
