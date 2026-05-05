// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EntropySource.hpp"

#include <string>

namespace erbsland::random::impl {

/// A POSIX entropy source reading bytes from an entropy device.
/// @tested{PosixEntropySourceTest}
class PosixEntropySource final : public EntropySource {
public:
    /// Create a POSIX entropy source.
    /// @param path The entropy device path.
    explicit PosixEntropySource(std::string path = "/dev/urandom");

    // defaults
    ~PosixEntropySource() override = default;
    PosixEntropySource(const PosixEntropySource &) = delete;
    auto operator=(const PosixEntropySource &) -> PosixEntropySource & = delete;

public: // implement EntropySource
    /// Fill the destination with entropy bytes from the configured device.
    /// @param destination The bytes to fill.
    /// @throws err::RandomError If the device cannot provide the requested bytes.
    void fillBytes(std::span<std::byte> destination) override;

private:
    std::string _path; ///< The entropy device path.
};

}
