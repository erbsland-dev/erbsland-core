// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "GHashFixture.hpp"

#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

GHashFixture::GHashFixture(el::Random &random, const bool population, bool accumulate, bool automatic) :
    _accumulate{accumulate}, _automatic{automatic} {

    const auto bytes = input(random, population, 16);
    const auto subkey = ci::GaloisMultiplier::Block::fromSpanOrThrow(bytes.span());
    const auto value =
        ci::GaloisMultiplier::Block::fromSpanOrThrow(el::ByteBuffer{el::ByteLength{16}, el::Byte{0x55U}}.span());
    auto multiplier = automatic ? ci::createGaloisMultiplier() : ci::createPortableGaloisMultiplier();
    require(multiplier->multiply(value, subkey) == ci::createPortableGaloisMultiplier()->multiply(value, subkey));
    if (accumulate) {
        auto actual =
            ci::GHash{subkey, automatic ? ci::createGaloisMultiplier() : ci::createPortableGaloisMultiplier()};
        auto expected = ci::GHash{subkey, ci::createPortableGaloisMultiplier()};
        actual.addCiphertext(value.span());
        expected.addCiphertext(value.span());
        require(actual.finalize() == expected.finalize());
    }
    _value = value;
    _subkey = subkey;
    _multiplier = std::move(multiplier);
}

template <bool tAccumulate>
auto GHashFixture::sample() -> uint64_t {
    constexpr auto accumulate = tAccumulate;

    if constexpr (!accumulate) {
        return checksum(_multiplier->multiply(_value, _subkey).span());
    } else {
        auto state =
            ci::GHash{_subkey, _automatic ? ci::createGaloisMultiplier() : ci::createPortableGaloisMultiplier()};
        state.addCiphertext(_value.span());
        return checksum(state.finalize().span());
    }
}

auto GHashFixture::measure(const uint64_t repetitions) -> uint64_t {
    if (_accumulate) {
        return repeat(repetitions, [this]() -> uint64_t { return sample<true>(); });
    }
    return repeat(repetitions, [this]() -> uint64_t { return sample<false>(); });
}

}
