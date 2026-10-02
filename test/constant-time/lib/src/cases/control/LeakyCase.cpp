// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LeakyCase.hpp"

#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

LeakyCase::LeakyCase() {
    configure(
        {"control/leaky"_el, "Deliberately data-dependent work"_el, "64-vs-128-iterations"_el}, el::ByteLength{128});
}

auto LeakyCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<LeakyCase>();
}

auto LeakyCase::createFixture([[maybe_unused]] el::Random &random, const bool population)
    -> std::unique_ptr<LeakyFixture> {
    return std::make_unique<LeakyFixture>(population);
}

}
