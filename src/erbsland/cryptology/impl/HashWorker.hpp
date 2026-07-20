// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HashAlgorithm.hpp"

#include "../../mem/ByteBlock_fwd.hpp"

#include <cstddef>
#include <memory>
#include <span>

namespace erbsland::cryptology::impl {

/// Private contract implemented by fixed-output hash workers.
/// Implementations must clone their complete streaming and finalized state.
/// @tested{HasherTest Sha3ValidationTest}
class HashWorker {
public:
    HashWorker() = default;
    HashWorker(const HashWorker &) = default;
    HashWorker(HashWorker &&) = default;
    auto operator=(const HashWorker &) -> HashWorker & = default;
    auto operator=(HashWorker &&) -> HashWorker & = default;
    virtual ~HashWorker() = default;

public:
    /// Clone the complete worker state for copy-on-write detachment.
    [[nodiscard]] virtual auto clone() const -> std::shared_ptr<HashWorker> = 0;
    /// Get the algorithm implemented by this worker.
    [[nodiscard]] virtual auto algorithm() const noexcept -> HashAlgorithm = 0;
    /// Reset the worker to its initial state.
    virtual void reset() = 0;
    /// Add bytes to the current stream.
    virtual void update(std::span<const std::byte> data) = 0;
    /// Finalize the stream or return the cached digest.
    [[nodiscard]] virtual auto finalize() -> mem::ByteBlock = 0;
};

}
