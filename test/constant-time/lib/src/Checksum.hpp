// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/MakeOneNamespace.hpp>

#include <atomic>
#include <cstdint>

namespace app::constant_time {

/// Opaque observable sink preventing elimination of repeated cryptographic operations.
/// @notest{Retained through real timing and control experiments.}
class Checksum final {
public:
    /// Access the process-local measurement sink.
    [[nodiscard]] static auto instance() noexcept -> Checksum &;
    /// Retain one operation result through an out-of-line store.
    /// @param value Observable operation output.
    void retain(uint64_t value) noexcept;

private:
    std::atomic<uint64_t> _value{}; ///< Last observable operation result.
};

}
