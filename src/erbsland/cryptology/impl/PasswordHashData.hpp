// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PasswordHashData_fwd.hpp"

#include "../PasswordHashPolicy.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"

#include <optional>

namespace erbsland::cryptology::impl {

/// Parsed immutable data behind a `PasswordHash`.
/// @tested{PasswordHasherTest}
class PasswordHashData final {
public:
    PasswordHashPolicy policy;                 ///< The parsed algorithm and cost policy.
    bool keyed{};                              ///< Whether the verifier is protected by an application key.
    std::optional<text::String> keyIdentifier; ///< The optional public key-rotation identifier.
    mem::ByteBlock salt;                       ///< The decoded public salt.
    mem::ByteBlock verifier;                   ///< The decoded stored verifier.
    text::String headerThroughSalt;            ///< The canonical authenticated header through the salt field.
    text::String canonical;                    ///< The complete canonical storage record.
};

}
