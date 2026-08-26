// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Environment.hpp"

#include "impl/Environment.hpp"

namespace erbsland::text::render {

auto Environment::create(EnvironmentOptions options) -> EnvironmentPtr {
    return std::make_shared<impl::Environment>(std::move(options));
}

}
