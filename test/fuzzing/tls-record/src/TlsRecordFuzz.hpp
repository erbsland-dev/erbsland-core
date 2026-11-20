// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/TlsCipherSuite.hpp>

#include <cstdint>
#include <span>

/// Exercise TLS record decryption for one fuzz input.
class TlsRecordFuzz final {
public:
    /// Create a harness for one input.
    /// @param data The bytes supplied by libFuzzer.
    explicit TlsRecordFuzz(const std::span<const uint8_t> data) : _data{data} {}

public: // operations
    /// Process the input once.
    /// @return Zero to continue fuzzing.
    [[nodiscard]] auto run() const -> int;

private:
    /// Select the cipher suite from the input byte.
    [[nodiscard]] static auto suiteFromSelector(const uint8_t selector) noexcept
        -> erbsland::cryptology::TlsCipherSuite;

private:
    std::span<const uint8_t> _data; ///< Suite selector followed by one exact record candidate.
};
