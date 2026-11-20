// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CueBoundary.hpp"

#include <erbsland/EventData.hpp>

namespace demo {

/// The queue owns this copy of the boundary until dispatch returns.
class CueEventData final : public el::EventData {
public:
    /// Wrap a cue boundary for delivery through the event loop.
    /// @param value The transition to deliver.
    explicit CueEventData(CueBoundary value) : _boundary{value} {}

public: // accessors
    /// The boundary borrowed by the display during dispatch.
    [[nodiscard]] auto boundary() const noexcept -> const CueBoundary & { return _boundary; }

private:
    CueBoundary _boundary; ///< Owned transition, independent of the timeline cursor.
};

}
