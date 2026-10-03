// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "algorithm/NistPrimeCurve.hpp"

#include "../../mem/ByteSpan.hpp"

namespace erbsland::cryptology::impl::ec_private_scalar_policy {

/// Reject extreme EC signing scalars during generation and private-key import.
/// The accepted interval is `L <= d <= n-L`, where `L = 2^(b-64)`.
/// @param bytes Big-endian private scalar, exactly 32 bytes for P-256 or 48 bytes for P-384.
/// @param curveName Selected curve, with bit width `b` and subgroup order `n`.
/// @return Whether the scalar satisfies the policy; this does not establish its unpredictability.
/// @seedoc{/reference/cryptology/key_management}
/// @tested{EcPrivateScalarPolicyTest EcSigningKeyTest}
[[nodiscard]] auto isAcceptable(mem::ConstByteSpan bytes, NistPrimeCurve::Name curveName) noexcept -> bool;

}
