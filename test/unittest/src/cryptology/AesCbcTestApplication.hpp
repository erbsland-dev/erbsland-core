// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DeterministicSecureRandom.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/impl/application_data/ApplicationRandomData.hpp>
#include <erbsland/core/impl/ApplicationData.hpp>

#include <memory>

namespace erbsland::test {

/// Application test double that installs deterministic random-fill entropy.
class AesCbcTestApplication final : public core::Application {
public:
    /// Create an application with deterministic secure randomness.
    AesCbcTestApplication() { _data->random().get()->setSecureRandom(std::make_unique<DeterministicSecureRandom>()); }
};

}
