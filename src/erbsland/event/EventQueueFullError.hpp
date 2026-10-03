// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/RuntimeError.hpp"

namespace erbsland::event {

/// An event could not be admitted to a full queue.
/// @tested{ApplicationEventTest}
class EventQueueFullError : public err::RuntimeError {
public:
    using err::RuntimeError::RuntimeError;
};

}
