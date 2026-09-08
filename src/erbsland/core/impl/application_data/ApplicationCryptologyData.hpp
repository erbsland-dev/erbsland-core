// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationCryptologyData_fwd.hpp"

#include "../../../cryptology/configuration/CryptologyConfiguration.hpp"

namespace erbsland::core::impl {

/// Application-wide cryptology configuration.
/// @tested{CryptologyConfigurationTest}
class ApplicationCryptologyData final {
public:
    /// Access the cryptology configuration.
    [[nodiscard]] auto configuration() noexcept -> cryptology::CryptologyConfiguration & { return _configuration; }

private:
    cryptology::CryptologyConfiguration _configuration; ///< Shared cryptology configuration.
};

}
