// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "HashFixture.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

using namespace el::text::literals;

HashFixture::HashFixture(el::Random &random, const bool population, el::HashAlgorithm algorithm) {
    auto data = input(random, population, 256);
    auto hasher = el::Hasher{algorithm};
    hasher.update(data.span());
    const auto expected = hasher.finalize();
    hasher.reset();
    hasher.update(data.span());
    require(expected == hasher.finalize());
    _data = std::move(data);
    _hasher = std::move(hasher);
}

auto HashFixture::sample() -> uint64_t {

    _hasher.reset();
    _hasher.update(_data.span());
    return checksum(_hasher.finalize().span());
}

auto HashFixture::measure(const uint64_t repetitions) -> uint64_t {
    return repeat(repetitions, [this]() -> uint64_t { return sample(); });
}

}
