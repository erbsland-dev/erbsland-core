// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "GHashCase.hpp"

#include "../../BackendInfo.hpp"

#include <erbsland/cryptology/impl/algorithm/aes/GaloisMultiplierFactory.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

GHashCase::GHashCase(bool accumulate, bool automatic) : _accumulate{accumulate}, _automatic{automatic} {
    const auto id = el::String{accumulate ? "ghash/accumulate"_el : "ghash/multiply"_el};
    const auto probe = automatic ? ci::createGaloisMultiplier() : ci::createPortableGaloisMultiplier();
    configure(
        TestMetadata{
            id,
            "GHASH field operation"_el,
            "fixed-vs-random-hash-subkey"_el,
            BackendInfo{*probe, !probe->isHardwareAccelerated()}.name(),
            true},
        el::ByteLength{2048});
}

auto GHashCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<GHashCase>(_accumulate, automatic);
}

auto GHashCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<GHashFixture> {
    return std::make_unique<GHashFixture>(random, population, _accumulate, _automatic);
}

}
