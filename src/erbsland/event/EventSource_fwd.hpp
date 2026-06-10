// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

class EventSource;
using EventSourcePtr = std::shared_ptr<EventSource>;
using EventSourceWeakPtr = std::weak_ptr<EventSource>;

}
