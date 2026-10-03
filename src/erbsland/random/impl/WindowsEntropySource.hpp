// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EntropySource.hpp"

namespace erbsland::random::impl {

/// A Windows entropy source using the preferred system RNG.
/// @tested{SecureRandomTest}
class WindowsEntropySource final : public EntropySource {
public:
    /// Create the Windows entropy source.
    WindowsEntropySource() = default;

    // defaults
    ~WindowsEntropySource() override = default;
    WindowsEntropySource(const WindowsEntropySource &) = delete;
    auto operator=(const WindowsEntropySource &) -> WindowsEntropySource & = delete;

public: // implement EntropySource
    /// Fill the destination with entropy bytes.
    /// The entropy source must be able to supply all requested bytes.
    /// @param destination The bytes to fill.
    void fillBytes(std::span<std::byte> destination) override;
};

}
