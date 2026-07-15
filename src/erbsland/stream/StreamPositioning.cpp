// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamPositioning.hpp"

#include "../text/Literals.hpp"

namespace erbsland::stream {

using namespace text::literals;

auto StreamPositioning::supportsPositioning() const noexcept -> bool {
    return false;
}

auto StreamPositioning::position() const -> unit::ByteIndex {
    throwError("Failed to get the stream position."_el, "This stream does not support positioning."_el);
}

auto StreamPositioning::setPosition([[maybe_unused]] const unit::ByteIndex position) -> StreamPositionStatus {
    throwError("Failed to set the stream position."_el, "This stream does not support positioning."_el);
}

auto StreamPositioning::movePosition(
    [[maybe_unused]] const StreamPositionOrigin origin, [[maybe_unused]] const unit::ByteOffset offset)
    -> StreamPositionStatus {
    throwError("Failed to move the stream position."_el, "This stream does not support positioning."_el);
}

}
