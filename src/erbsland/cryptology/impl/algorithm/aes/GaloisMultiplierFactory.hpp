// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GaloisMultiplier.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

/// Create the fastest permitted supported GHASH multiplier, with a portable fallback.
/// The application-wide acceleration setting is observed when this worker is constructed.
/// @return The selected field multiplier.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest CryptologyConfigurationTest}
[[nodiscard]] auto createGaloisMultiplier() -> std::unique_ptr<GaloisMultiplier>;
/// Create the portable GHASH multiplier for reference testing.
/// @return The portable field multiplier.
/// @tested{AesPrimitiveTest}
[[nodiscard]] auto createPortableGaloisMultiplier() -> std::unique_ptr<GaloisMultiplier>;

}
