// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartManager.hpp"

#include "impl/ApplicationPartManager.hpp"

namespace erbsland::core {

auto ApplicationPartManager::create(event::EventsPtr controlEvents) -> ApplicationPartManagerPtr {
    return impl::ApplicationPartManager::create(std::move(controlEvents));
}

}
