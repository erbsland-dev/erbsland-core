// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

/// Shared state connecting one event sender with its receivers.
/// @notest{This placeholder contains no behavior yet.}
template <typename T>
class EventPipe {};

template <typename T>
using EventPipePtr = std::shared_ptr<EventPipe<T>>;

}
