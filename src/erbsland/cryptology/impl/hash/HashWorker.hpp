// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../HashAlgorithm.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

/// Private contract implemented by fixed-output hash workers.
/// Implementations must clone their complete streaming and finalized state.
/// @tested{HasherTest Sha3ValidationTest}
class HashWorker {
public:
    // defaults
    HashWorker() = default;
    virtual ~HashWorker() = default;
    HashWorker(const HashWorker &) = default;
    HashWorker(HashWorker &&) = default;
    auto operator=(const HashWorker &) -> HashWorker & = default;
    auto operator=(HashWorker &&) -> HashWorker & = default;

public:
    /// Clone the complete worker state for copy-on-write detachment.
    /// @return A separately owned worker with identical state.
    [[nodiscard]] virtual auto clone() const -> std::shared_ptr<HashWorker> = 0;
    /// Get the algorithm implemented by this worker.
    [[nodiscard]] virtual auto algorithm() const noexcept -> HashAlgorithm = 0;
    /// Reset the worker to its initial state.
    virtual void reset() = 0;
    /// Securely erase message-dependent state and reset the worker.
    virtual void secureErase() noexcept = 0;
    /// Add bytes to the current stream.
    /// @param data The next exact message bytes.
    virtual void update(mem::ConstByteSpan data) = 0;
    /// Finalize the stream or return the cached digest.
    /// @return The fixed-size digest.
    [[nodiscard]] virtual auto finalize() -> mem::ByteBlock = 0;
};

}
