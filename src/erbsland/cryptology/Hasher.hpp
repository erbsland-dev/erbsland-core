// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashAlgorithm.hpp"

#include "impl/hash/HashWorker_fwd.hpp"

#include "../mem/Byte.hpp"
#include "../mem/ByteBlock_fwd.hpp"
#include "../mem/ByteSpan.hpp"
#include "../text/String_fwd.hpp"

#include <memory>

namespace erbsland::cryptology {

/// A state object for calculating a fixed-output cryptographic hash.
/// A default-constructed hasher is invalid and acts as a placeholder. Copies use copy-on-write state: they initially
/// share a worker and detach before mutation. After finalization, the cached digest remains available until `reset()`;
/// calling `update()` before resetting is a logic error.
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{HasherTest Sha3ValidationTest HashValidationTest HashFullValidationTest}
class Hasher final {
public:
    /// Create an invalid placeholder.
    Hasher() noexcept = default;
    /// Create a hasher for an algorithm.
    /// @param algorithm The fixed-output hash algorithm.
    explicit Hasher(HashAlgorithm algorithm);

    // defaults
    ~Hasher();
    Hasher(const Hasher &);
    Hasher(Hasher &&) noexcept;
    /// Copy another hasher into this hasher.
    auto operator=(const Hasher &) -> Hasher &;
    /// Move another hasher into this hasher.
    auto operator=(Hasher &&) noexcept -> Hasher &;

public:
    /// Reset the worker to hash a new stream with the same algorithm.
    /// This hasher must be valid.
    void reset();
    /// Securely erase message-dependent state and reset this hasher.
    void secureErase();
    /// Add Erbsland Core byte data.
    /// Empty spans are accepted.
    /// This hasher must be valid and must not have been finalized.
    /// @param data The next exact message bytes.
    void update(mem::ConstByteSpan data);
    /// Add a byte block.
    /// This hasher must be valid and must not have been finalized.
    /// @param data The next exact message bytes.
    void update(const mem::ByteBlock &data);
    /// Add the exact bytes in a UTF-8 string's internal buffer.
    /// No validation, normalization, byte order mark, or encoding conversion is performed.
    /// This hasher must be valid and must not have been finalized.
    /// @param text The UTF-8 text whose stored bytes are added.
    void update(const text::String &text);
    /// Finalize the current stream and return its digest.
    /// Repeated calls return the cached digest.
    /// This hasher must be valid.
    /// @return The fixed-size digest.
    [[nodiscard]] auto finalize() -> mem::ByteBlock;

public: // tests
    /// Test if this hasher has an algorithm and worker state.
    [[nodiscard]] auto isValid() const noexcept -> bool;

public: // accessors
    /// Get the configured algorithm.
    /// This hasher must be valid.
    [[nodiscard]] auto algorithm() const -> HashAlgorithm;

private:
    /// Get a uniquely owned worker, detaching shared state when necessary.
    [[nodiscard]] auto workerForWrite() -> impl::HashWorker &;
    /// Throw if this is an invalid placeholder.
    void requireValid() const;

private:
    std::shared_ptr<impl::HashWorker> _worker; ///< The shared copy-on-write worker state.
};

}
