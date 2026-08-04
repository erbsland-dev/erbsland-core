// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

/// Receiving endpoint for a typed event pipe.
/// @notest{This placeholder contains no behavior yet.}
template <typename T>
class EventPipeReceiver {};

template <typename T>
using EventPipeReceiverPtr = std::shared_ptr<EventPipeReceiver<T>>;

}
