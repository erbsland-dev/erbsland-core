// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

class Events;
using EventsPtr = std::shared_ptr<Events>;
using EventsWeakPtr = std::weak_ptr<Events>;

}
