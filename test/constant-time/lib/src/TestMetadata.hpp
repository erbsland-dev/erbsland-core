// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/text/String.hpp>

namespace app::constant_time {

using namespace el::text::literals;
/// Description of one fixed-shape timing experiment.
/// @tested{ConstantTimeRunnerTest}
struct TestMetadata final {
    el::String id{};                              ///< Stable selector.
    el::String description{};                     ///< Target operation.
    el::String populations{"fixed-vs-random"_el}; ///< Explicit population distinction.
    el::String backend{"portable"_el};            ///< Actual selected backend.
    bool backendVariants{};                       ///< Whether the operation can use platform acceleration.
};

}
