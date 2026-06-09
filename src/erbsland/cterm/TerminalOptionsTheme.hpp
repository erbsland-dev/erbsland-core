// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStyle.hpp"

namespace erbsland::cterm {

/// Theme for terminal rendering of command line option output.
/// @tested{TerminalOptionsRendererTest}
class TerminalOptionsTheme final {
public:
    TerminalOptionsTheme() = default;

    // defaults
    ~TerminalOptionsTheme() = default;
    TerminalOptionsTheme(const TerminalOptionsTheme &) = default;
    TerminalOptionsTheme(TerminalOptionsTheme &&) = default;
    auto operator=(const TerminalOptionsTheme &) -> TerminalOptionsTheme & = default;
    auto operator=(TerminalOptionsTheme &&) -> TerminalOptionsTheme & = default;

public: // accessors
    /// Get the style for usage labels.
    [[nodiscard]] auto usage() const noexcept -> BlockStyle { return _usage; }
    /// Set the style for usage labels.
    void setUsage(const BlockStyle style) noexcept { _usage = style; }
    /// Get the style for program and module names.
    [[nodiscard]] auto programName() const noexcept -> BlockStyle { return _programName; }
    /// Set the style for program and module names.
    void setProgramName(const BlockStyle style) noexcept { _programName = style; }
    /// Get the style for section headings.
    [[nodiscard]] auto heading() const noexcept -> BlockStyle { return _heading; }
    /// Set the style for section headings.
    void setHeading(const BlockStyle style) noexcept { _heading = style; }
    /// Get the style for option names.
    [[nodiscard]] auto optionName() const noexcept -> BlockStyle { return _optionName; }
    /// Set the style for option names.
    void setOptionName(const BlockStyle style) noexcept { _optionName = style; }
    /// Get the style for value placeholders.
    [[nodiscard]] auto valueName() const noexcept -> BlockStyle { return _valueName; }
    /// Set the style for value placeholders.
    void setValueName(const BlockStyle style) noexcept { _valueName = style; }
    /// Get the style for metadata labels.
    [[nodiscard]] auto label() const noexcept -> BlockStyle { return _label; }
    /// Set the style for metadata labels.
    void setLabel(const BlockStyle style) noexcept { _label = style; }
    /// Get the style for default values.
    [[nodiscard]] auto defaultValue() const noexcept -> BlockStyle { return _defaultValue; }
    /// Set the style for default values.
    void setDefaultValue(const BlockStyle style) noexcept { _defaultValue = style; }
    /// Get the style for descriptions and normal text.
    [[nodiscard]] auto description() const noexcept -> BlockStyle { return _description; }
    /// Set the style for descriptions and normal text.
    void setDescription(const BlockStyle style) noexcept { _description = style; }
    /// Get the style for error output.
    [[nodiscard]] auto error() const noexcept -> BlockStyle { return _error; }
    /// Set the style for error output.
    void setError(const BlockStyle style) noexcept { _error = style; }

public:
    /// Get the shared default theme.
    [[nodiscard]] static auto defaultTheme() noexcept -> const TerminalOptionsTheme &;

private:
    BlockStyle _usage{fg::BrightWhite};       ///< Style for usage labels.
    BlockStyle _programName{fg::BrightGreen}; ///< Style for program and module names.
    BlockStyle _heading{fg::BrightWhite};     ///< Style for section headings.
    BlockStyle _optionName{fg::BrightCyan};   ///< Style for option names.
    BlockStyle _valueName{fg::Magenta};       ///< Style for value placeholders.
    BlockStyle _label{fg::BrightWhite};       ///< Style for metadata labels.
    BlockStyle _defaultValue{fg::BrightBlue}; ///< Style for default values.
    BlockStyle _description{fg::White};       ///< Style for descriptions and normal text.
    BlockStyle _error{fg::BrightRed};         ///< Style for error output.
};

}
