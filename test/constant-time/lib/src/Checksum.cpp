// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Checksum.hpp"

namespace app::constant_time {

auto Checksum::instance() noexcept -> Checksum & {
    static auto sink = Checksum{};
    return sink;
}

void Checksum::retain(const uint64_t value) noexcept {
    _value.store(value, std::memory_order_relaxed);
    std::atomic_signal_fence(std::memory_order_seq_cst);
}

}
