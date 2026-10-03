// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../RegEx_fwd.hpp"

#include "../../text/String.hpp"
#include "../../text/StringLiteral.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../util/FifoCache.hpp"
#include "../../util/LruCache.hpp"

#include <mutex>

namespace erbsland::re::impl {

/// Thread-safe, best-effort retention of compiled literal patterns.
/// Compilation occurs outside the lock; concurrent misses may compile the same pattern more than once.
/// @tested{RegExCachedTest}
class RegExCache final {
public:
    /// Create a cache with independent FIFO and LRU limits.
    /// @param fifoCapacity Maximum entries in the fast insertion-order cache.
    /// @param lruCapacity Maximum entries in the backing access-order cache.
    explicit RegExCache(
        unit::ItemCount fifoCapacity = unit::ItemCount{64}, unit::ItemCount lruCapacity = unit::ItemCount{512});

public: // compilation
    /// Obtain a compiled expression, retaining only successful compilations.
    /// @param pattern Literal pattern compiled with default settings.
    /// @return Owning immutable compiled expression.
    /// @throws RegExError If compilation fails.
    [[nodiscard]] auto compile(text::StringLiteral pattern) -> ConstRegExPtr;

private:
    /// Look up and promote an entry while the caller holds the mutex.
    auto find(const text::String &pattern) -> ConstRegExPtr;

private:
    std::mutex _mutex;                                  ///< Serializes lookups and retention.
    util::FifoCache<text::String, ConstRegExPtr> _fifo; ///< Recent insertions, read without refreshing order.
    util::LruCache<text::String, ConstRegExPtr> _lru;   ///< Recency of accesses reaching the backing cache.
};

}
