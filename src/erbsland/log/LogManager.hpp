// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogConfiguration.hpp"
#include "LogManager_fwd.hpp"
#include "LogManagerOptions.hpp"
#include "LogManagerStatistics.hpp"
#include "LogStream_fwd.hpp"
#include "LogTraceSection.hpp"

#include "impl/LogManagerData_fwd.hpp"

#include "../core/Application_fwd.hpp"

namespace erbsland::log {

/// Background logging manager.
/// @tested{LogCoreTest LogWriterTest}
class LogManager final {
    friend class core::Application;
    struct PrivateTag {};

private:
    /// Restricts manager construction to the shared factory.
    class ConstructionToken final {
        friend class LogManager;

        /// Create a private construction token.
        ConstructionToken() = default;
    };

public:
    /// Internal constructor used by `create()`.
    /// @param options The validated queue and shutdown limits for the manager.
    explicit LogManager(LogManagerOptions options, PrivateTag);
    /// Shut down the worker during destruction.
    ~LogManager();

    // defaults/deletions
    LogManager(const LogManager &) = delete;
    LogManager(LogManager &&) = delete;
    auto operator=(const LogManager &) -> LogManager & = delete;
    auto operator=(LogManager &&) -> LogManager & = delete;

public:
    /// Create and start a shared logging manager.
    /// @param options The queue, message, and shutdown limits to use.
    /// @return A running manager with a root stream and no writer routes.
    [[nodiscard]] static auto create(LogManagerOptions options = {}) -> LogManagerPtr;
    /// Access the root producer stream.
    [[nodiscard]] auto rootStream() const noexcept -> const LogStreamPtr &;
    /// Create a stream for a validated path.
    /// @param path The validated hierarchical path for the stream.
    /// @param traceSection The optional trace section controlling trace emission.
    /// @return A new lightweight producer stream owned independently of the manager.
    [[nodiscard]] auto createStream(LogPath path, LogTraceSection traceSection = {}) -> LogStreamPtr;
    /// Validate a path and create its stream.
    /// @param path The lowercase ASCII slash-delimited stream path.
    /// @param traceSection The optional trace section controlling trace emission.
    /// @return A new lightweight producer stream owned independently of the manager.
    [[nodiscard]] auto createStream(const text::String &path, LogTraceSection traceSection = {}) -> LogStreamPtr;
    /// Synchronously replace the active configuration.
    /// Entries already inside a writer complete with the old configuration. Queued entries use the replacement.
    /// @param configuration The complete replacement configuration.
    /// @throws err::LogicError If shutdown started or a writer is active in another manager.
    void setConfiguration(LogConfiguration configuration);
    /// Copy the active configuration snapshot.
    /// @return A copy of the configuration currently used by the worker.
    [[nodiscard]] auto configuration() const -> LogConfiguration;
    /// Pause ordinary worker draining while retaining queued entries.
    void pause() noexcept;
    /// Resume worker draining.
    void resume() noexcept;
    /// Gracefully stop the worker within the configured deadline.
    void shutdown() noexcept;
    /// Read a point-in-time statistics snapshot.
    /// @return The counters accumulated by the manager at the time of the call.
    [[nodiscard]] auto statistics() const noexcept -> LogManagerStatistics;

private:
    /// Install a writer that survives ordinary configuration replacement.
    /// @param writer The writer to install persistently.
    /// @param filter The route filter for the persistent writer.
    /// @throws err::LogicError If shutdown started, the writer is empty, or it belongs to another manager.
    void addPersistentWriter(LogWriterPtr writer, LogWriterFilter filter);

private:
    impl::LogManagerDataPtr _data; ///< Shared manager state and worker implementation.
};

}
