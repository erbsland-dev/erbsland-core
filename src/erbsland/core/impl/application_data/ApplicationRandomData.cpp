// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationRandomData.hpp"

#include "../../../random/Random.hpp"
#include "../../../random/SecureRandom.hpp"
#include "../../../random/ThreadSafeFastRandom.hpp"

namespace erbsland::core::impl {

ApplicationRandomData::ApplicationRandomData() :
    _random{std::make_unique<random::ThreadSafeFastRandom>()}, _secureRandom{std::make_unique<random::SecureRandom>()} {
}

ApplicationRandomData::~ApplicationRandomData() = default;

auto ApplicationRandomData::random() noexcept -> random::Random & {
    return *_random;
}

auto ApplicationRandomData::secureRandom() noexcept -> random::Random & {
    return *_secureRandom;
}

#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
void ApplicationRandomData::setRandom(random::RandomPtr random) {
    _random = random != nullptr ? std::move(random) : std::make_unique<random::ThreadSafeFastRandom>();
}

void ApplicationRandomData::setSecureRandom(random::RandomPtr secureRandom) {
    _secureRandom = secureRandom != nullptr ? std::move(secureRandom) : std::make_unique<random::SecureRandom>();
}
#endif

}
