// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamErrorSource.hpp"
#include "StreamPositionOrigin.hpp"
#include "StreamPositionStatus.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteOffset.hpp"

namespace erbsland::stream {

/// Optional byte-based positioning for streams.
/// Positioning is a stable capability of a stream. The logical position excludes native input read-ahead and includes
/// output accepted for delivery. Unsupported operations throw `StreamError`.
/// @tested{StreamPositionTest}
class StreamPositioning : public virtual StreamErrorSource {
public:
    virtual ~StreamPositioning() = default;

public: // positioning
    /// Test if this stream supports positioning.
    [[nodiscard]] virtual auto supportsPositioning() const noexcept -> bool;
    /// Get the logical byte position.
    /// @throws stream::StreamError If positioning is not supported.
    [[nodiscard]] virtual auto position() const -> unit::ByteIndex;
    /// Set the logical byte position.
    /// @param position The absolute byte position. Positions beyond the current end are allowed.
    /// @return `Success`, or `Timeout` if the operation could not be serialized before the deadline.
    /// @throws err::ParameterError If `position` is invalid or outside native file-offset bounds.
    /// @throws stream::StreamError If positioning is unsupported or fails.
    virtual auto setPosition(unit::ByteIndex position) -> StreamPositionStatus;
    /// Move the logical byte position.
    /// @param origin The reference point for the movement.
    /// @param offset The signed byte offset from `origin`.
    /// @return `Success`, or `Timeout` if the operation could not be serialized before the deadline.
    /// @throws err::ParameterError If the result is negative or outside native file-offset bounds.
    /// @throws stream::StreamError If positioning is unsupported or fails.
    virtual auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus;
};

}
