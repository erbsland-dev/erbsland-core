// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionModule_fwd.hpp"
#include "OptionSet_fwd.hpp"
#include "OptionValue_fwd.hpp"
#include "OptionValues_fwd.hpp"

#include "../unit/ExitCode.hpp"

#include <functional>

namespace erbsland::options {

/// A callback to validate option values.
/// Option values are validated before any post-parsing callback is called.
/// The first argument of the callback is the value to be validated.
/// @throws err::OptionError if the validation failed.
///     Must provide `description` in the error context.
///     Missing values are automatically added by the surrounding context.
/// @tested{OptionsParserTest}.
using OptionValidateFn = std::function<void(OptionValuePtr valueToValidate, OptionValuesPtr values)>;
/// A callback called before an option set is parsed.
/// @throws err::OptionError if parsing should be aborted.
///     Must provide `description` in the error context.
///     Can optionally provide `option` to pin-down an option as error-location.
///     Missing values are automatically added by the surrounding context.
/// @tested{OptionsFrameworkTest}
using PreOptionSetParsingFn = std::function<void(OptionSetPtr)>;
/// A callback called before an option module is parsed.
/// @throws err::OptionError if parsing should be aborted.
///     Must provide `description` in the error context.
///     Can optionally provide `option` or `optionSet` to pin-down an option as error-location.
///     Missing values are automatically added by the surrounding context.
/// @tested{OptionsFrameworkTest}
using PreOptionModuleParsingFn = std::function<void(OptionModulePtr)>;
/// A callback called after successful parsing.
/// @throws err::OptionError if post-validation failed.
///     Must provide `description` in the error context.
///     Shall provide `option` or `optionSet` to pin-down the error-location.
///     Missing values are automatically added by the surrounding context.
/// @tested{OptionsFrameworkTest}
using PostParsingFn = std::function<void(OptionValuesPtr)>;
/// The main function for a selected option module.
/// @note For convenience, exceptions derived from `err::Exception` are automatically handled in `core::Application`.
///     The exception message is printed to stdOut(), and the program exits with code 1.
///     We recommend that exceptions shall be handled inside the function, and it returns a custom exit code.
/// @throws err::Exception Exceptions derived from `err::Exception` are displayed,
///     and the application exits with error code 1.
/// @tested{OptionsFrameworkTest}
using ModuleMainFn = std::function<unit::ExitCode(OptionValuesPtr)>;

}
