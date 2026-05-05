// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionErrorContext_fwd.hpp"
#include "OptionRenderer_fwd.hpp"
#include "OptionResult.hpp"
#include "Options_fwd.hpp"

#include "../unit/ArgumentUnit.hpp"

namespace erbsland::options {

/// Renders option help, version text, and error messages for a chosen backend.
/// @tested{OptionsFrameworkTest}
class OptionRenderer {
public:
    OptionRenderer() = default;
    virtual ~OptionRenderer() = default;
    OptionRenderer(const OptionRenderer &) = default;
    auto operator=(const OptionRenderer &) -> OptionRenderer & = default;
    OptionRenderer(OptionRenderer &&) = default;
    auto operator=(OptionRenderer &&) -> OptionRenderer & = default;

public:
    /// Display help for the given options.
    /// @param options The options to display help for.
    /// @param moduleName The name of the module to display help for.
    ///     Empty if options have no modules, or the help flag was called without a module name.
    virtual void displayHelp(const OptionsPtr &options, text::StringView moduleName);
    /// Display version information for the application or module.
    /// @param options The options object as reference. Can be ignored.
    /// @param moduleName The name of the module to display version information for.
    ///     Empty if options have no modules, or the version flag was called without a module name.
    virtual void displayVersion(const OptionsPtr &options, text::StringView moduleName);
    /// Display an option processing error.
    /// @param options The options object as reference. Can be ignored.
    /// @param errorContext The error context containing information about the error.
    virtual void displayError(const OptionsPtr &options, const OptionErrorContext &errorContext);
};

}
