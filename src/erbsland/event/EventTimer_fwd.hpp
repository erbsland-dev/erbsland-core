// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

class EventTimer;
using EventTimerPtr = std::shared_ptr<EventTimer>;
using EventTimerWeakPtr = std::weak_ptr<EventTimer>;

}
