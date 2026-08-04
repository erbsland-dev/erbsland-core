// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HmacWorker_fwd.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../HashAlgorithm.hpp"

namespace erbsland::cryptology::impl {

/// Private contract implemented by the supported HMAC workers.
/// @tested{HmacTest HashPrimitiveFullValidationTest}
class HmacWorker {
public:
    // defaults
    HmacWorker() = default;
    virtual ~HmacWorker() = default;
    HmacWorker(const HmacWorker &) = default;
    HmacWorker(HmacWorker &&) = default;
    auto operator=(const HmacWorker &) -> HmacWorker & = default;
    auto operator=(HmacWorker &&) -> HmacWorker & = default;

public:
    /// Get the underlying hash algorithm.
    [[nodiscard]] virtual auto algorithm() const noexcept -> HashAlgorithm = 0;
    /// Reset the message while retaining the key.
    virtual void reset() = 0;
    /// Securely erase all retained state.
    virtual void secureErase() noexcept = 0;
    /// Add exact message bytes.
    /// @param data The next authenticated message bytes.
    virtual void update(mem::ConstByteSpan data) = 0;
    /// Finalize the message or return the cached authenticator.
    /// @return The complete HMAC value.
    [[nodiscard]] virtual auto finalize() -> mem::ByteBlock = 0;
};

}
