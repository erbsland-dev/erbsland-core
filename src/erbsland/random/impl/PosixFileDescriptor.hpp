// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string>

namespace erbsland::random::impl {

/// A small RAII wrapper for a POSIX file descriptor.
/// @tested{PosixEntropySourceTest}
class PosixFileDescriptor final {
public:
    /// Open a file descriptor for reading.
    /// @param path The path to open.
    /// @throws err::RandomError If the path cannot be opened.
    explicit PosixFileDescriptor(const std::string &path);

    /// Close the descriptor.
    ~PosixFileDescriptor();

    // defaults
    PosixFileDescriptor(const PosixFileDescriptor &) = delete;
    auto operator=(const PosixFileDescriptor &) -> PosixFileDescriptor & = delete;

public:
    /// Access the raw descriptor.
    /// @return The raw POSIX file descriptor.
    [[nodiscard]] auto descriptor() const noexcept -> int { return _descriptor; }

private:
    int _descriptor{-1}; ///< The raw POSIX file descriptor.
};

}
