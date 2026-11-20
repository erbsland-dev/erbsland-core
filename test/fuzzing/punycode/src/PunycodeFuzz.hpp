// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

/// Exercise Punycode encoding or decoding for one fuzz input.
class PunycodeFuzz final {
public:
    /// Create a harness for one input.
    /// @param data The bytes supplied by libFuzzer.
    explicit PunycodeFuzz(const std::span<const uint8_t> data) : _data{data} {}

public: // operations
    /// Process the input once.
    /// @return Zero to continue fuzzing.
    [[nodiscard]] auto run() const -> int;

private:
    static constexpr auto cMaximumInputSize = std::size_t{4096U}; ///< Maximum size of one fuzz input.
    std::span<const uint8_t> _data;                               ///< Input bytes supplied by libFuzzer.
};
