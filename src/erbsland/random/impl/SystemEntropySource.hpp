// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EntropySource.hpp"

#include <memory>

namespace erbsland::random::impl {

/// The system entropy source selected for the current platform.
/// @tested{SecureRandomTest}
class SystemEntropySource final : public EntropySource {
public:
    /// Create the system entropy source.
    SystemEntropySource();
    /// Destroy the system entropy source.
    ~SystemEntropySource() override;

    // defaults
    SystemEntropySource(const SystemEntropySource &) = delete;
    auto operator=(const SystemEntropySource &) -> SystemEntropySource & = delete;

public: // implement EntropySource
    /// Fill the destination with system entropy bytes.
    /// @param destination The bytes to fill.
    /// @throws random::RandomError If the system entropy source cannot provide the requested bytes.
    void fillBytes(std::span<std::byte> destination) override;

private:
    std::unique_ptr<EntropySource> _source; ///< The platform entropy adapter.
};

}
