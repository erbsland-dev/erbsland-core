// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ActiveLogConfiguration_fwd.hpp"
#include "ActiveLogWriter.hpp"

#include "../LogConfiguration.hpp"
#include "../LogManagerOptions.hpp"
#include "../LogTraceSection.hpp"

#include <vector>

namespace erbsland::log::impl {

/// Immutable runtime data transformed from a user-facing log configuration.
/// @tested{ActiveLogConfigurationTest LogCoreTest LogRouteFormatTest}
class ActiveLogConfiguration final {
public:
    /// Copy and prepare the configuration elements needed for active logging.
    /// @param configuration The fluent configuration whose independent runtime data shall be prepared.
    explicit ActiveLogConfiguration(const LogConfiguration &configuration);

public: // accessors
    /// Access the active resource limits.
    [[nodiscard]] auto managerOptions() const noexcept -> const LogManagerOptions & { return _managerOptions; }
    /// Access the ordered runtime routes.
    [[nodiscard]] auto writers() const noexcept -> const std::vector<ActiveLogWriter> & { return _writers; }
    /// Access unique writer instances in first-route order for binding and shutdown.
    [[nodiscard]] auto uniqueWriters() const noexcept -> const std::vector<LogWriterPtr> & { return _uniqueWriters; }
    /// Test whether an optional trace section is enabled.
    /// @param section The section to look up.
    /// @return `true` for an unnamed section or a configured section.
    [[nodiscard]] auto isTraceSectionEnabled(const LogTraceSection &section) const noexcept -> bool;

private:
    LogManagerOptions _managerOptions;           ///< Runtime queue and shutdown limits.
    std::vector<LogTraceSection> _traceSections; ///< Enabled trace sections.
    std::vector<ActiveLogWriter> _writers;       ///< Ordered runtime routes with compiled formatter pointers.
    std::vector<LogWriterPtr> _uniqueWriters;    ///< Unique writer instances used for lifecycle operations.
};

}
