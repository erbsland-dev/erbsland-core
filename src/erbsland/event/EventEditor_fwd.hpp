// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

class EventEditor;
using EventEditorPtr = std::shared_ptr<EventEditor>;
using EventEditorWeakPtr = std::weak_ptr<EventEditor>;

}
