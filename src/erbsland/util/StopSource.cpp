// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StopSource.hpp"

namespace erbsland::util {

StopSource::StopSource() : _state{std::make_shared<std::atomic<bool>>(false)} {
}

auto StopSource::stopToken() const noexcept -> StopToken {
    return StopToken{_state};
}

auto StopSource::isStopRequested() const noexcept -> bool {
    return stopToken().isStopRequested();
}

auto StopSource::requestStop() noexcept -> bool {
    return _state != nullptr && !_state->exchange(true, std::memory_order_acq_rel);
}

}
