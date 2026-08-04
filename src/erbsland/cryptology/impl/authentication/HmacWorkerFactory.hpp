// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HmacWorker_fwd.hpp"

#include "../../../mem/ByteSpan.hpp"
#include "../../HashAlgorithm.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

/// Test whether an algorithm is supported by HMAC and HKDF.
/// @param algorithm The hash algorithm to test.
/// @return `true` for SHA-256 or SHA-384.
/// @tested{HmacTest HkdfTest}
[[nodiscard]] auto isHmacAlgorithmSupported(HashAlgorithm algorithm) noexcept -> bool;

/// Create a built-in HMAC worker.
/// @param algorithm The supported underlying hash algorithm.
/// @param key The exact secret key bytes.
/// @return A new keyed worker.
/// @throws err::ParameterError If the algorithm is unsupported.
/// @tested{HmacTest HashPrimitiveFullValidationTest}
[[nodiscard]] auto createHmacWorker(HashAlgorithm algorithm, mem::ConstByteSpan key) -> std::unique_ptr<HmacWorker>;

}
