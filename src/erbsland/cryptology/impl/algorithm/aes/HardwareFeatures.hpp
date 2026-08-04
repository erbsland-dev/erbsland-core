// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::cryptology::impl::hardware_features {

/// Test whether the current x86-64 processor supports AES-NI.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
[[nodiscard]] auto hasX86Aes() noexcept -> bool;
/// Test whether the current x86-64 processor supports PCLMULQDQ.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
[[nodiscard]] auto hasX86Pclmul() noexcept -> bool;
/// Test whether the current ARM64 processor supports FEAT_AES.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
[[nodiscard]] auto hasArmAes() noexcept -> bool;
/// Test whether the current ARM64 processor supports FEAT_PMULL.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
[[nodiscard]] auto hasArmPmull() noexcept -> bool;

}
