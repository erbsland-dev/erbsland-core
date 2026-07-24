// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParameterDefinition_fwd.hpp"
#include "ParameterSelection.hpp"
#include "ParameterType.hpp"

#include <erbsland/all.hpp>

#include <optional>

namespace erbsland::profiling {

/// A registered typed scenario parameter.
/// @tested{ProfilingDefinitionTest}
struct ParameterDefinition {
    String id;                                               ///< Stable internal identifier.
    String configurationName;                                ///< ELCL field name.
    ParameterType type{ParameterType::Text};                 ///< Required value type.
    std::optional<ParameterSelection::Value> defaultValue{}; ///< Optional default value.
};

}
