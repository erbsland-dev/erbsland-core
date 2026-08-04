// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricDecryptorBackendAccess_fwd.hpp"
#include "SymmetricDecryptorData.hpp"

#include "../../symmetric/SymmetricDecryptor.hpp"

#include <memory>
#include <utility>

namespace erbsland::cryptology::impl {

/// Internal bridge for wrapping a symmetric decryption worker in its public facade.
/// @tested{SymmetricEncryptionFrontendTest}
class SymmetricDecryptorBackendAccess final {
public:
    /// Create access for a worker that shall be wrapped in a facade.
    explicit SymmetricDecryptorBackendAccess(std::unique_ptr<SymmetricDecryptorData> data) noexcept :
        _data{std::move(data)} {}

public:
    /// Create the public facade and transfer worker ownership into it.
    [[nodiscard]] auto create() noexcept -> SymmetricDecryptor { return SymmetricDecryptor{std::move(_data)}; }

private:
    std::unique_ptr<SymmetricDecryptorData> _data; ///< The worker to wrap.
};

}
