// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Literals.hpp>

namespace erbsland::profiling::configuration_key {

using namespace el::text::literals;

/// Profiling configuration value `methods`.
constexpr auto cMethodsValue = "methods"_el;
/// Profiling configuration value `name`.
constexpr auto cNameValue = "name"_el;
/// Profiling configuration value `seed`.
constexpr auto cSeedValue = "seed"_el;
/// Profiling configuration value `sensitive_mode`.
constexpr auto cSensitiveModeValue = "sensitive_mode"_el;
/// Profiling configuration value `threads`.
constexpr auto cThreadsValue = "threads"_el;
/// Profiling configuration value `variants`.
constexpr auto cVariantsValue = "variants"_el;

}
