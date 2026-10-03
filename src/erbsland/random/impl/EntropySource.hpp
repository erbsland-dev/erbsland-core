// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <span>

namespace erbsland::random::impl {

/// A source of cryptographic entropy.
/// @tested{SecureRandomTest}
class EntropySource {
public:
    // defaults
    virtual ~EntropySource() = default;

    // defaults
    EntropySource(const EntropySource &) = delete;
    auto operator=(const EntropySource &) -> EntropySource & = delete;

public:
    /// Fill the destination with entropy bytes.
    /// The entropy source must be able to supply all requested bytes.
    /// @param destination The bytes to fill.
    virtual void fillBytes(std::span<std::byte> destination) = 0;

protected:
    /// Create an entropy source.
    EntropySource() = default;
};

}
