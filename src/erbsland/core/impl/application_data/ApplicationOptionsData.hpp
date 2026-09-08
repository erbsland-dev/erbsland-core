// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationOptionsData_fwd.hpp"

#include "../../../options/Options.hpp"
#include "../../../options/OptionSensitiveTextLocation.hpp"
#include "../../../options/OptionValues.hpp"
#include "../../CommandLineArguments.hpp"

#include <atomic>

namespace erbsland::core::impl {

/// Command-line option definitions, arguments, and parsed values for an application.
/// @tested{ApplicationOptionsTest}
class ApplicationOptionsData final {
public:
    /// Create application options with an empty default option set.
    ApplicationOptionsData();

public:
    /// Set and convert borrowed narrow command-line arguments from `main()`.
    void setCommandLineArguments(int argc, char *argv[]);
    /// Set and convert borrowed wide command-line arguments from `wmain()`.
    void setCommandLineArguments(int argc, wchar_t *argv[]);
    /// Access the converted command-line arguments.
    [[nodiscard]] auto commandLineArguments() const noexcept -> const CommandLineArguments &;
    /// Access mutable arguments for option parsing and sensitive-text masking.
    [[nodiscard]] auto commandLineArgumentsForParsing() noexcept -> CommandLineArguments &;
    /// Mask sensitive suffixes in the borrowed native argument vector.
    void maskSensitiveCommandLineText(const options::OptionSensitiveTextLocations &locations) noexcept;
    /// Access the configured command-line options.
    [[nodiscard]] auto options() noexcept -> const options::OptionsPtr &;
    /// Release the configured command-line options.
    void releaseOptions() noexcept;
    /// Access the resolved option values.
    [[nodiscard]] auto optionValues() noexcept -> const options::OptionValuesPtr &;
    /// Replace the resolved option values.
    void setOptionValues(options::OptionValuesPtr optionValues) noexcept;

private:
    template <typename Char>
    /// Mask sensitive native command-line arguments.
    static void maskNativeArguments(
        int argumentCount, Char **arguments, const options::OptionSensitiveTextLocations &locations) noexcept;

private:
    std::atomic<bool> _argumentsInitialized{false}; ///< Whether command-line arguments were initialized.
    CommandLineArguments _arguments;                ///< Converted command-line arguments.
    int _nativeArgumentCount{0};                    ///< Original native argument count.
    char **_nativeArguments{nullptr};               ///< Borrowed original narrow argument vector.
    wchar_t **_nativeWideArguments{nullptr};        ///< Borrowed original wide argument vector.
    options::OptionsPtr _options;                   ///< Global option definitions.
    options::OptionValuesPtr _optionValues;         ///< Parsed option values.
};

}
