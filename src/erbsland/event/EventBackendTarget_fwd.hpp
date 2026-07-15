// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

class EventBackendTarget;
using EventBackendTargetPtr = std::shared_ptr<EventBackendTarget>;
using EventBackendTargetWeakPtr = std::weak_ptr<EventBackendTarget>;

}
