// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventSource_fwd.hpp"

namespace erbsland::event {

/// Base interface for event sources that can be registered with an event loop.
/// @notest{Marker interface; concrete event sources provide the observable behavior.}
class EventSource {
public:
    virtual ~EventSource() = default;

public:
};

}
