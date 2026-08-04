// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DeterministicSecureRandom.hpp"

#include <erbsland/core/Application.hpp>

#include <memory>

namespace erbsland::test {

/// Application test double that installs deterministic random-fill entropy.
class AesCbcTestApplication final : public core::Application {
protected:
    void initializeSecureRandom(random::RandomPtr &randomPtr) noexcept override {
        randomPtr = std::make_unique<DeterministicSecureRandom>();
    }
};

}
