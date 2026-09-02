// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../PlatformErrorContext_fwd.hpp"

#include "../../text/String.hpp"

namespace erbsland::system::impl {

/// A cached error for one process-information query.
/// @tested{ProcessInfoTest}
struct ProcessInfoError final {
    text::String reason;                  ///< Portable description of the failed query.
    PlatformErrorContextConstPtr context; ///< Captured native error context.

    /// Test if this value represents an error.
    [[nodiscard]] auto hasError() const noexcept -> bool { return !reason.isEmpty(); }
};

}
