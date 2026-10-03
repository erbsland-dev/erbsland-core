// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StopToken.hpp"

#include <utility>

namespace erbsland::util {

StopToken::StopToken(std::shared_ptr<std::atomic<bool>> state) : _state{std::move(state)} {
}

auto StopToken::isStopRequested() const noexcept -> bool {
    return _state != nullptr && _state->load(std::memory_order_acquire);
}

}
