// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ScenarioTemplate.hpp"

#include "../Configuration.hpp"

#include <erbsland/conf/Value.hpp>

namespace app::stream::impl {

/// Parse a profiling configuration document with optional default values.
[[nodiscard]] auto parseConfigurationDocument(
    const el::conf::ValuePtr &document, const Configuration *defaults = nullptr) -> Configuration;

}
