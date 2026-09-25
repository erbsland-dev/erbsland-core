// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// Select a Unicode normalization form.
/// @seedoc{/topics/text_strings/normalizing_strings}
enum class NormalizationForm : uint8_t {
    Nfc = 0, ///< Canonical decomposition followed by canonical composition.
    Nfd,     ///< Canonical decomposition without composition.
    Nfkc,    ///< Compatibility decomposition followed by canonical composition.
    Nfkd,    ///< Compatibility decomposition without composition.
};

}
