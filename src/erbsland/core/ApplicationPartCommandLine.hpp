// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../options/Options_fwd.hpp"
#include "../options/OptionValues_fwd.hpp"

namespace erbsland::core {

/// Command-line lifecycle hooks implemented by application parts.
/// @tested{ApplicationPartManagerTest ApplicationPartApplicationTest}
class ApplicationPartCommandLine {
public:
    // defaults
    virtual ~ApplicationPartCommandLine() = default;

public:
    /// Register this part's command-line options.
    /// @param options The shared application option definitions.
    virtual void registerCommandLineOptions(const options::OptionsPtr &options) = 0;
    /// Receive successfully parsed command-line values.
    /// @param values The parsed option values.
    virtual void parseCommandLine(const options::OptionValuesPtr &values) = 0;
};

}
