// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "HashCase.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

HashCase::HashCase(el::HashAlgorithm algorithm) : _algorithm{algorithm} {
    const auto id = el::StringFormat{"hash/{}"_el}.build(algorithm.toString());

    configure(TestMetadata{id, "Fixed-output hash of equal-length sensitive input"_el}, el::ByteLength{8192});
}

auto HashCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<HashCase>(_algorithm);
}

auto HashCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<HashFixture> {
    return std::make_unique<HashFixture>(random, population, _algorithm);
}

}
