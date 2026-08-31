// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogConfiguration.hpp"

#include "../conf/Integer.hpp"
#include "../conf/Value_fwd.hpp"
#include "../conf/vr/Rules_fwd.hpp"
#include "../cterm/Terminal_fwd.hpp"

#include <utility>

namespace erbsland::log {

/// Parse and validate a logging configuration from any selected ELCL branch.
/// @tested{LogConfigurationParserTest}
class LogConfigurationParser final {
public:
    /// Create a parser with an optional terminal for console-writer configuration.
    /// @param terminal The terminal used by configured console writers, or empty to reject console writers.
    explicit LogConfigurationParser(cterm::TerminalPtr terminal = {}) noexcept : _terminal{std::move(terminal)} {}

    /// Access the compiled validation rules shared by all parser instances.
    /// @return The immutable compiled rules for a selected logging configuration branch.
    [[nodiscard]] static auto validationRules() -> const conf::vr::RulesPtr &;
    /// Get the current version of the log configuration format.
    [[nodiscard]] static auto version() -> conf::Integer;
    /// Validate and parse one document or section branch.
    /// @param sectionValue The selected ELCL section containing logging configuration values.
    /// @return A complete configuration with newly constructed built-in writers.
    /// @throws conf::ConfError If the selected branch fails the compiled validation rules.
    [[nodiscard]] auto parse(const conf::ValuePtr &sectionValue) const -> LogConfiguration;

private:
    /// Parse one built-in writer definition.
    /// @param value The validated writer configuration object.
    /// @return A newly constructed built-in writer.
    [[nodiscard]] auto parseWriter(const conf::ValuePtr &value) const -> LogWriterPtr;
    /// Parse one writer route filter.
    /// @param value The validated route configuration object.
    /// @return The parsed level and path filter.
    [[nodiscard]] static auto parseFilter(const conf::ValuePtr &value) -> LogWriterFilter;

private:
    cterm::TerminalPtr _terminal; ///< Optional terminal supplied to configured console writers.
};

}
