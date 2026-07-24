// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Application_fwd.hpp"

#include <erbsland/profiling/ProfilingApplication.hpp>

namespace app::conf {

/// The declarative ELCL parser profiling application.
/// @tested{ConfigurationLoaderTest}
class ConfParserProfileApplication final : public erbsland::profiling::ProfilingApplication {
public:
    using ProfilingApplication::ProfilingApplication;

protected: // implement ProfilingApplication
    void configureProfiling(erbsland::profiling::ProfilingDefinition &definition) override;
    [[nodiscard]] auto executeProfiling() -> erbsland::ExitCode override;
};

}
