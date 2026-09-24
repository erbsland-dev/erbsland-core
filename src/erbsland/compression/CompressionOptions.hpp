// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ByteLength.hpp"

namespace erbsland::compression {
/// Compressor memory policy.
/// @tested{CompressionStreamingTest}
class CompressionOptions final {
public:
    /// Get the maximum codec workspace.
    auto maximumWorkspaceLength() const noexcept -> unit::ByteLength { return _maximumWorkspaceLength; }
    /// Set the finite maximum codec workspace.
    auto setMaximumWorkspaceLength(unit::ByteLength value) -> CompressionOptions & {
        _maximumWorkspaceLength = value;
        return *this;
    }

private:
    unit::ByteLength _maximumWorkspaceLength{256U * 1024U * 1024U}; ///< Codec allocation limit.
};
}
