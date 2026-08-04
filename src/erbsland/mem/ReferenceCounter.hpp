// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <exception>
#include <limits>

namespace erbsland::mem {

/// An atomic reference counter.
///
/// @warning This is an advanced data type, meant for people extending the library.
/// Do not use it unless you understand the implications and have a specific need.
class ReferenceCounter final {
public:
    /// The reference status for humans
    enum ReferenceStatus : uint8_t { NoReferences, HasReferences };

public: // defaults
    ReferenceCounter() = default;
    ~ReferenceCounter() = default;
    ReferenceCounter(const ReferenceCounter &) = delete;
    ReferenceCounter(ReferenceCounter &&) = delete;
    auto operator=(const ReferenceCounter &) -> ReferenceCounter & = delete;
    auto operator=(ReferenceCounter &&) -> ReferenceCounter & = delete;

public:
    /// Add a reference.
    /// @return The referencing status of the counter.
    auto addReference() noexcept -> ReferenceStatus {
        auto count = _counter.load(std::memory_order_relaxed);
        while (true) {
            if (count == std::numeric_limits<uint32_t>::max()) {
                failInvariant();
            }
            // Another thread may change the counter between the load and the update. If that happens,
            // `compare_exchange_weak` fails and writes the latest observed counter value back into `count`; the next
            // loop iteration then retries with that fresh value. It may also fail spuriously, so the retry loop is
            // required even when no other thread updates the counter.
            if (_counter.compare_exchange_weak(
                    count, count + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
                return HasReferences;
            }
        }
    }
    /// Remove a reference.
    /// @return The referencing status of the counter.
    auto removeReference() noexcept -> ReferenceStatus {
        auto count = _counter.load(std::memory_order_relaxed);
        while (true) {
            if (count == 0) {
                failInvariant();
            }
            const auto newCount = count - 1;
            // The decrement must only succeed for the value we checked above. If another thread changed the counter,
            // `compare_exchange_weak` writes the new observed value back to `count`, and the loop checks it again for
            // underflow before retrying. The acquire/release ordering synchronizes the last release with destruction.
            if (_counter.compare_exchange_weak(count, newCount, std::memory_order_acq_rel, std::memory_order_relaxed)) {
                return newCount == 0 ? NoReferences : HasReferences;
            }
        }
    }
    /// Checks if the object is referenced.
    [[nodiscard]] auto isReferenced() const noexcept -> bool { return _counter.load(std::memory_order_relaxed) != 0; }
    /// Get the referencing status of the counter.
    [[nodiscard]] auto status() const noexcept -> ReferenceStatus {
        return isReferenced() ? HasReferences : NoReferences;
    }
    /// Checks if the object is shared with multiple instances.
    [[nodiscard]] auto isShared() const noexcept -> bool { return useCount() > 1; }
    /// Get the current number of references.
    [[nodiscard]] auto useCount() const noexcept -> uint32_t { return _counter.load(std::memory_order_relaxed); }

private:
    /// Terminate after detecting a reference-counter overflow or underflow.
    [[noreturn]] static void failInvariant() noexcept {
        assert(false && "ReferenceCounter invariant violation.");
        std::terminate();
    }

private:
    std::atomic<uint32_t> _counter{0}; ///< The counter.
};

}
