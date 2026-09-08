// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationRandomData.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../random/Random.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::core::impl {

using namespace text::literals;

auto ApplicationRandomData::random(const Factory &factory) -> random::Random & {
    const auto lock = std::scoped_lock{_mutex};
    if (_random == nullptr) {
        _random = factory();
        if (_random == nullptr) {
            throw err::LogicError{"The application random factory returned a null pointer."_el};
        }
    }
    return *_random;
}

auto ApplicationRandomData::secureRandom(const Factory &factory) -> random::Random & {
    const auto lock = std::scoped_lock{_mutex};
    if (_secureRandom == nullptr) {
        _secureRandom = factory();
        if (_secureRandom == nullptr) {
            throw err::LogicError{"The application secure-random factory returned a null pointer."_el};
        }
    }
    return *_secureRandom;
}

}
