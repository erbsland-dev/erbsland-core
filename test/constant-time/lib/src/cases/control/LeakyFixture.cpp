// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LeakyFixture.hpp"

namespace app::constant_time {

using namespace el::text::literals;

auto LeakyFixture::sample() const -> uint64_t {
    volatile uint64_t value = 1;
    for (auto index = 0; index < (_population ? 128 : 64); ++index) {
        value = value * 33U + 1U;
    }
    return value;
}

auto LeakyFixture::measure(const uint64_t repetitions) -> uint64_t {
    return repeat(repetitions, [this]() -> uint64_t { return sample(); });
}

}
