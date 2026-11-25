// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ItemCount.hpp>
#include <erbsland/String.hpp>

#include <optional>

namespace tool::frame {

/// One consistent view of a task's current phase.
struct TaskProgressSnapshot final {
    el::String phase;                   ///< Human-readable work description.
    el::ItemCount completed;            ///< Completed work items in this phase.
    std::optional<el::ItemCount> total; ///< Known item count, absent during discovery.
};

}
