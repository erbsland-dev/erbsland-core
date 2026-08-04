// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

/// Sending endpoint for a typed event pipe.
/// @notest{This placeholder contains no behavior yet.}
template <typename T>
class EventPipeSender {};

template <typename T>
using EventPipeSenderPtr = std::shared_ptr<EventPipeSender<T>>;

}
