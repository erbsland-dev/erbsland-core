// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ScenarioTemplate_fwd.hpp"

#include "../ProfileTypes.hpp"

namespace app::string::impl {

/// Unexpanded string-profiler scenario configuration.
/// @notest{Covered by string profiler configuration CTest entries.}
struct ScenarioTemplate {
    el::String name;
    std::vector<StringWidth> widths;
    std::vector<StringType> types;
    std::vector<UseCase> useCases;
    std::vector<el::String> variants;
    std::vector<ContentProfile> contentProfiles{ContentProfile::Mixed};
    std::vector<SizeMode> sizeModes{SizeMode::Fixed};
    std::uint64_t sizeMinimum{4096U};
    std::uint64_t sizeMaximum{4096U};
    std::uint64_t sizeStep{1U};
    std::uint64_t operandSizeMinimum{64U};
    std::uint64_t operandSizeMaximum{64U};
    std::vector<SensitiveSelection> sensitiveModes;
    std::uint32_t weight{1U};
    bool allWidths{true};
    bool allTypes{true};
    bool allUseCases{true};
    bool allVariants{true};
    bool allContentProfiles{};
};

}
