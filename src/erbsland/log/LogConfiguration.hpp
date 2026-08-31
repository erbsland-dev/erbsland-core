// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLineFormat.hpp"
#include "LogManagerOptions.hpp"
#include "LogTraceSection.hpp"
#include "LogWriter_fwd.hpp"
#include "LogWriterFilter.hpp"

#include "impl/LogManagerData_fwd.hpp"
#include "impl/LogWriterBinding.hpp"

#include <cstddef>
#include <vector>

namespace erbsland::log {

/// A complete immutable-by-snapshot manager configuration.
/// @tested{LogCoreTest LogWriterTest}
class LogConfiguration final {
    friend class impl::LogManagerData;

public:
    /// Create a configuration without writer routes.
    LogConfiguration() = default;

public:
    /// Access the line-format settings.
    [[nodiscard]] auto lineFormat() const noexcept -> const LogLineFormat & { return _lineFormat; }
    /// Replace the line-format settings.
    /// @param value The shared formatting settings used by every writer route.
    /// @return This configuration for chained option construction.
    auto setLineFormat(LogLineFormat value) noexcept -> LogConfiguration &;
    /// Access the manager resource limits.
    [[nodiscard]] auto managerOptions() const noexcept -> const LogManagerOptions & { return _managerOptions; }
    /// Validate and replace the manager resource limits.
    /// @param value The complete manager limits to validate and store.
    /// @return This configuration for chained option construction.
    auto setManagerOptions(LogManagerOptions value) -> LogConfiguration &;
    /// Get the number of configured writer routes.
    /// @return The number of writer and filter pairs in this configuration.
    [[nodiscard]] auto writerCount() const noexcept -> std::size_t { return _writers.size(); }
    /// Add a writer route.
    /// @param writer The writer instance receiving accepted entries.
    /// @param filter The level and path filter evaluated before delivery.
    /// @return This configuration for chained route construction.
    auto addWriter(LogWriterPtr writer, LogWriterFilter filter = {}) -> LogConfiguration &;
    /// Access explicitly enabled trace sections.
    [[nodiscard]] auto traceSections() const noexcept -> const std::vector<LogTraceSection> & { return _traceSections; }
    /// Enable a trace section.
    /// @param section The case-sensitive trace section to enable.
    /// @return This configuration for chained option construction.
    auto enableTraceSection(LogTraceSection section) -> LogConfiguration &;
    /// Test whether the given trace section is enabled.
    /// @param section The trace section to look up.
    /// @return `true` if the section is present in the enabled-section list.
    [[nodiscard]] auto isTraceSectionEnabled(const LogTraceSection &section) const noexcept -> bool;
    /// Test whether a trace entry would be accepted by any route.
    /// @param path The path of the stream being evaluated.
    /// @param section The optional trace section of the stream.
    /// @return `true` if the section is enabled when named and at least one route accepts trace for `path`.
    [[nodiscard]] auto acceptsTrace(const LogPath &path, const LogTraceSection &section) const noexcept -> bool;

private:
    /// Access implementation-only writer routes.
    /// @return The ordered writer bindings.
    [[nodiscard]] auto writerBindings() const noexcept -> const std::vector<impl::LogWriterBinding> & {
        return _writers;
    }

private:
    LogLineFormat _lineFormat;                    ///< Shared line-format settings.
    LogManagerOptions _managerOptions;            ///< Manager queue and shutdown limits.
    std::vector<impl::LogWriterBinding> _writers; ///< Ordered implementation-only writer routes.
    std::vector<LogTraceSection> _traceSections;  ///< Case-sensitive enabled trace sections.
};

}
