// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompressionPhase.hpp"
#include "CompressionProgressAction.hpp"
#include "CompressionTransferResult.hpp"

#include <functional>
#include <optional>

namespace erbsland::compression {
/// Progress of a synchronous compression transfer.
/// @tested{CompressionStreamingTest}
struct CompressionProgress final {
    CompressionTransferResult transferred;                ///< Cumulative I/O counts.
    std::optional<unit::ByteLength> inputTotal;           ///< Exact input length, if supplied.
    std::optional<unit::ByteLength> outputTotal;          ///< Expected output length, if known.
    CompressionPhase phase{CompressionPhase::Processing}; ///< Current phase.
};
/// Synchronous observer; cancellation is honored before completion.
using CompressionProgressFn = std::function<CompressionProgressAction(const CompressionProgress &)>;
}
