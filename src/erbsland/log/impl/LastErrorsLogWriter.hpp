// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../LogWriter.hpp"

#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

namespace erbsland::log::impl {

/// Retains the most recent error entries.
/// @tested{LogWriterTest}
class LastErrorsLogWriter final : public LogWriter {
public:
    /// Create an error snapshot writer with the given FIFO capacity.
    /// @param capacity The maximum number of recent error entries to retain.
    explicit LastErrorsLogWriter(std::size_t capacity = 25U);

public: // implements LogWriter
    void write(const LogEntryConstPtr &entry, const LogLineConstPtr &line) override;

public:
    /// Share the retained immutable entries in FIFO order.
    /// @return A thread-safe pointer snapshot ordered from oldest to newest.
    [[nodiscard]] auto snapshot() const -> std::vector<LogEntryConstPtr>;
    /// Get the maximum retained entry count.
    [[nodiscard]] auto capacity() const noexcept -> std::size_t { return _capacity; }

private:
    std::size_t _capacity{25U};            ///< Maximum number of retained error entries.
    mutable std::mutex _mutex;             ///< Protects snapshot storage.
    std::deque<LogEntryConstPtr> _entries; ///< Retained error entries ordered from oldest to newest.
};

}
