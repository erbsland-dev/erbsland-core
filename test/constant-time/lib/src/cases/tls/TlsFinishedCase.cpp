// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TlsFinishedCase.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

TlsFinishedCase::TlsFinishedCase(el::HashAlgorithm algorithm) : _algorithm{algorithm} {
    const auto tlsId = el::StringFormat{"tls/finished/{}"_el}.build(el::HashAlgorithm{algorithm}.toString());

    configure(
        TestMetadata{tlsId, "TLS Finished verification"_el, "first-vs-last-invalid-finished"_el},
        el::ByteLength{65536});
}

auto TlsFinishedCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<TlsFinishedCase>(_algorithm);
}

auto TlsFinishedCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<TlsFinishedFixture> {
    return std::make_unique<TlsFinishedFixture>(random, population, _algorithm);
}

}
