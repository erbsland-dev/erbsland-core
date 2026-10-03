// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ScriptedSecureRandom.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/core/impl/application_data/ApplicationRandomData.hpp>
#include <erbsland/core/impl/ApplicationData.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/ProtectedDataMode.hpp>

#include <memory>
#include <utility>

namespace erbsland::test {

/// Application-local scripted entropy for EC generation tests.
class EcSigningKeyTestApplication final : public core::Application {
public:
    /// Install the fixture during construction, before application randomness becomes accessible.
    EcSigningKeyTestApplication() {
        auto source = std::make_unique<ScriptedSecureRandom>();
        _source = source.get();
        _data->random().get()->setSecureRandom(std::move(source));
        cryptologyConfiguration().setProtectedDataMode(cryptology::ProtectedDataMode::InternalOnly);
    }

public: // fixture access
    /// Access the entropy script owned by this application.
    [[nodiscard]] auto scriptedRandom() noexcept -> ScriptedSecureRandom & { return *_source; }

private:
    ScriptedSecureRandom *_source{}; ///< Borrowed fixture; application random data owns it.
};

}
