// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::syntax {

/// The word pair used for Boolean literals.
enum class BooleanForm : uint8_t {
    Keep,            ///< Preserve the original word pair.
    TrueFalse,       ///< Write true and false.
    YesNo,           ///< Write yes and no.
    EnabledDisabled, ///< Write enabled and disabled.
    OnOff,           ///< Write on and off.
};

}
