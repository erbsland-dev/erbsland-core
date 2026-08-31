// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../time/TimeDelta.hpp"
#include "../unit/ByteLength.hpp"

#include <cstddef>

namespace erbsland::log {

/// Resource limits for a log manager.
/// @tested{LogCoreTest LogConfigurationParserTest}
class LogManagerOptions final {
public:
    /// Get the maximum number of queued entries.
    /// @return The complete entry capacity including reserved entries.
    [[nodiscard]] auto maximumEntries() const noexcept -> std::size_t { return _maximumEntries; }
    /// Set the maximum number of queued entries.
    /// @param value The positive complete entry capacity.
    /// @return These options for chained configuration.
    auto setMaximumEntries(std::size_t value) -> LogManagerOptions &;
    /// Get the maximum number of bytes retained in the queue.
    /// @return The complete byte capacity including reserved bytes.
    [[nodiscard]] auto maximumBytes() const noexcept -> unit::ByteLength { return _maximumBytes; }
    /// Set the maximum number of bytes retained in the queue.
    /// @param value The positive complete byte capacity.
    /// @return These options for chained configuration.
    auto setMaximumBytes(unit::ByteLength value) -> LogManagerOptions &;
    /// Get the entry slots reserved for warning and error entries.
    /// @return The reserved entry capacity.
    [[nodiscard]] auto reservedErrorEntries() const noexcept -> std::size_t { return _reservedErrorEntries; }
    /// Set the entry slots reserved for warning and error entries.
    /// @param value The reserved entry capacity, which must not exceed `maximumEntries()` when installed.
    /// @return These options for chained configuration.
    auto setReservedErrorEntries(std::size_t value) noexcept -> LogManagerOptions &;
    /// Get the bytes reserved for warning and error entries.
    /// @return The reserved byte capacity.
    [[nodiscard]] auto reservedErrorBytes() const noexcept -> unit::ByteLength { return _reservedErrorBytes; }
    /// Set the bytes reserved for warning and error entries.
    /// @param value The reserved bytes, which must not exceed `maximumBytes()` when installed.
    /// @return These options for chained configuration.
    auto setReservedErrorBytes(unit::ByteLength value) noexcept -> LogManagerOptions &;
    /// Get the maximum sanitized size of one message.
    /// @return The positive producer-side message limit.
    [[nodiscard]] auto maximumMessageBytes() const noexcept -> unit::ByteLength { return _maximumMessageBytes; }
    /// Set the maximum sanitized size of one message.
    /// @param value The positive producer-side message limit.
    /// @return These options for chained configuration.
    auto setMaximumMessageBytes(unit::ByteLength value) -> LogManagerOptions &;
    /// Get the graceful shutdown deadline.
    /// @return The positive duration available for draining queued entries.
    [[nodiscard]] auto shutdownTimeout() const noexcept -> time::TimeDelta { return _shutdownTimeout; }
    /// Set the graceful shutdown deadline.
    /// @param value The positive duration available for draining queued entries.
    /// @return These options for chained configuration.
    auto setShutdownTimeout(time::TimeDelta value) -> LogManagerOptions &;

private:
    std::size_t _maximumEntries{4096U};                            ///< Maximum number of queued entries.
    unit::ByteLength _maximumBytes{8U * 1024U * 1024U};            ///< Maximum bytes retained in the queue.
    std::size_t _reservedErrorEntries{256U};                       ///< Entry slots reserved for warnings and errors.
    unit::ByteLength _reservedErrorBytes{1024U * 1024U};           ///< Queue bytes reserved for warnings and errors.
    unit::ByteLength _maximumMessageBytes{256U * 1024U};           ///< Maximum sanitized message size.
    time::TimeDelta _shutdownTimeout{time::TimeDelta::seconds(2)}; ///< Graceful shutdown deadline.
};

}
