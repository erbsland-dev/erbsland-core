// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HmacWorker.hpp"

#include "../algorithm/HmacAlgorithm.hpp"

#include "../../../mem/ByteBlock.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// Adapts one concrete HMAC implementation to the private worker contract.
/// @tparam tHash The concrete hash implementation.
/// @tparam tAlgorithm The public hash-algorithm value.
/// @tparam tBlockSize The hash compression-block size.
/// @tparam tMaximumMessageLength The maximum accepted message length.
/// @tested{HmacTest HashPrimitiveFullValidationTest}
template <typename tHash, HashAlgorithm::Value tAlgorithm, std::size_t tBlockSize, uint64_t tMaximumMessageLength>
class HmacWorkerAdapter final : public HmacWorker {
public:
    /// Create a worker for a secret key.
    /// @param key The exact secret key bytes.
    explicit HmacWorkerAdapter(const mem::ConstByteSpan key) : _hmac{key} {}

public: // implement HmacWorker
    /// Get the adapted hash algorithm.
    [[nodiscard]] auto algorithm() const noexcept -> HashAlgorithm override { return tAlgorithm; }
    /// Reset the message while retaining the key.
    void reset() override { _hmac.reset(); }
    /// Securely erase all retained state.
    void secureErase() noexcept override { _hmac.secureErase(); }
    /// Add exact message bytes.
    /// @param data The next authenticated message bytes.
    void update(const mem::ConstByteSpan data) override { _hmac.update(data); }
    /// Finalize the message or return the cached authenticator.
    /// @return The complete HMAC value.
    [[nodiscard]] auto finalize() -> mem::ByteBlock override { return mem::ByteBlock{_hmac.finalize()}; }

private:
    HmacAlgorithm<tHash, tBlockSize, tMaximumMessageLength> _hmac; ///< The concrete keyed state.
};

}
