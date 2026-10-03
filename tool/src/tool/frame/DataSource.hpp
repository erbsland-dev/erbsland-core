// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ToolApplication_fwd.hpp"

#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/LogStream.hpp>

namespace tool::frame {

/// A configured, application-owned source of verified input data.
/// @tested{DataSourceTest}
class DataSource {
    friend class ToolApplication;

public:
    // defaults/deletions
    DataSource() = default;
    virtual ~DataSource() = default;
    DataSource(const DataSource &) = delete;
    void operator=(const DataSource &) = delete;

public: // configuration and initialization
    /// Get optional rules for the section named by the concrete source's static identifier.
    /// @return Rules requiring a section, or null for a source without configuration.
    [[nodiscard]] virtual auto configurationRules() const -> el::conf::vr::RulesPtr;
    /// Interpret validated configuration before loading any data.
    /// @param configuration Section with defaults applied, or null when there are no rules.
    virtual void configure(const el::conf::ValuePtr &configuration);
    /// Load and validate this source; successful return makes it available to tools.
    virtual void initialize() = 0;

protected: // logging
    /// Access logging assigned before configuration and initialization.
    /// Application logging must have been assigned.
    /// @return The application's root log stream.
    [[nodiscard]] auto log() const -> const el::LogStreamPtr &;

private:
    /// Assign the application log stream before source callbacks.
    /// @param log The stream shared with the selected command.
    void setLog(el::LogStreamPtr log);

private:
    el::LogStreamPtr _log; ///< Logging for source loading and diagnostics.
};

}
