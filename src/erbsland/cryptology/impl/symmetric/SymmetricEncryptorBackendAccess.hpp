// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricEncryptorBackendAccess_fwd.hpp"
#include "SymmetricEncryptorData.hpp"

#include "../../symmetric/SymmetricEncryptor.hpp"

#include <memory>
#include <utility>

namespace erbsland::cryptology::impl {

/// Internal bridge for wrapping a symmetric encryption worker in its public facade.
/// @tested{SymmetricEncryptionFrontendTest}
class SymmetricEncryptorBackendAccess final {
public:
    /// Create access for a worker that shall be wrapped in a facade.
    explicit SymmetricEncryptorBackendAccess(std::unique_ptr<SymmetricEncryptorData> data) noexcept :
        _data{std::move(data)} {}

public:
    /// Create the public facade and transfer worker ownership into it.
    [[nodiscard]] auto create() noexcept -> SymmetricEncryptor { return SymmetricEncryptor{std::move(_data)}; }

private:
    std::unique_ptr<SymmetricEncryptorData> _data; ///< The worker to wrap.
};

}
