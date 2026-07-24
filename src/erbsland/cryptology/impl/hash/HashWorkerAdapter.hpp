// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashWorker.hpp"

#include "../../../mem/ByteBlock.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

/// Adapt a fixed-output streaming hash implementation to the public hash worker contract.
/// @tparam tHash The concrete hash implementation.
/// @tparam tAlgorithm The public algorithm value.
/// @tested{HasherTest}
template <typename tHash, HashAlgorithm::Value tAlgorithm>
class HashWorkerAdapter final : public HashWorker {
public:
    // defaults
    HashWorkerAdapter() = default;

public: // implement HashWorker
    /// Clone the complete adapted hash state.
    [[nodiscard]] auto clone() const -> std::shared_ptr<HashWorker> override {
        return std::make_shared<HashWorkerAdapter>(*this);
    }
    /// Get the algorithm fixed by `tAlgorithm`.
    [[nodiscard]] auto algorithm() const noexcept -> HashAlgorithm override { return tAlgorithm; }
    /// Reset the adapted hash state.
    void reset() override { _hash.reset(); }
    /// Securely erase message-dependent state and reset the adapted hash.
    void secureErase() noexcept override { _hash.secureErase(); }
    /// Add bytes to the adapted hash state.
    /// @param data The next exact message bytes.
    void update(const mem::ConstByteSpan data) override { _hash.update(data); }
    /// Finalize the adapted hash and copy its fixed-size digest into a byte block.
    [[nodiscard]] auto finalize() -> mem::ByteBlock override {
        const auto digest = _hash.digest();
        return mem::ByteBlock{digest};
    }

private:
    tHash _hash; ///< The adapted hash state.
};

}
