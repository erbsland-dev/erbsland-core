// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../core/Definitions.hpp"

namespace erbsland::cryptology::unsafe {

/// An explicit acknowledgement that password hashes will be stored without an application pepper.
/// @seedoc{/reference/cryptology/password_hashing}
/// @tested{PasswordHasherTest}
class UnsafeNoPasswordHashKey final {
public:
    /// Explicitly acknowledge the reduced protection.
    [[nodiscard]] static constexpr auto acknowledgeRisk() noexcept -> UnsafeNoPasswordHashKey { return {}; }

private:
    // defaults
    constexpr UnsafeNoPasswordHashKey() noexcept = default;
};

}
