// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/List.hpp>
#include <erbsland/LogWriter.hpp>

namespace tool::pre_commit::impl {

/// Retain visible task messages; read only after the owning manager has shut down.
class TaskLogWriter final : public el::LogWriter {
public:
    /// Select whether informational and trace messages are retained for display.
    /// @param verbose Include all levels when true.
    explicit TaskLogWriter(bool verbose) : _verbose{verbose} {}

public: // implement LogWriter
    void write(const el::LogEntryConstPtr &entry, const el::log::LogLineConstPtr &line) override;

public: // completed capture
    /// Access entries after the owning manager has drained and joined.
    [[nodiscard]] auto entries() const -> const el::List<el::LogEntryConstPtr> & { return _entries; }
    /// Test whether any error-level entries were delivered.
    [[nodiscard]] auto hasErrors() const noexcept -> bool { return _errors; }
    /// Test whether any warning-level entries were delivered.
    [[nodiscard]] auto hasWarnings() const noexcept -> bool { return _warnings; }
    /// Test whether any captured message was truncated.
    [[nodiscard]] auto hasTruncation() const noexcept -> bool { return _truncated; }

private:
    bool _verbose;                           ///< Retain routine output only on request.
    bool _errors{};                          ///< Delivered error state.
    bool _warnings{};                        ///< Delivered warning state.
    bool _truncated{};                       ///< Producer truncation state.
    el::List<el::LogEntryConstPtr> _entries; ///< Entries written by the manager's sole worker.
};

}
