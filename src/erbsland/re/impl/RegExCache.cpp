// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegExCache.hpp"

#include "../RegEx.hpp"

namespace erbsland::re::impl {

RegExCache::RegExCache(unit::ItemCount fifoCapacity, unit::ItemCount lruCapacity) :
    _fifo{fifoCapacity}, _lru{lruCapacity} {
}

auto RegExCache::find(const text::String &pattern) -> ConstRegExPtr {
    if (const auto found = _fifo.get(pattern)) {
        return *found;
    }
    if (const auto found = _lru.get(pattern)) {
        _fifo.set(pattern, *found);
        return *found;
    }
    return {};
}

auto RegExCache::compile(text::StringLiteral pattern) -> ConstRegExPtr {
    const auto key = text::String{pattern};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (auto found = find(key)) {
            return found;
        }
    }
    auto compiled = RegEx::compile(key);
    const auto lock = std::scoped_lock{_mutex};
    if (auto found = find(key)) {
        return found;
    }
    _lru.set(key, compiled);
    _fifo.set(key, compiled);
    return compiled;
}

}
