// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/log/all.hpp>

#include <condition_variable>
#include <mutex>
#include <stdexcept>

namespace demo {

/// A demo writer that holds its first write until explicitly released.
/// @notest{This support type is exercised by the compiled logging topic demos.}
class BlockingLogWriter final : public el::log::LogWriter {
public: // implement LogWriter
    void write(
        [[maybe_unused]] const el::log::LogEntryConstPtr &entry,
        [[maybe_unused]] const el::log::LogLineConstPtr &line) override {
        auto lock = std::unique_lock{_mutex};
        _writing = true;
        _condition.notify_all();
        _condition.wait(lock, [this]() -> bool { return _released; });
    }

public:
    /// Wait until the manager worker enters the writer.
    void waitUntilWriting() {
        auto lock = std::unique_lock{_mutex};
        _condition.wait(lock, [this]() -> bool { return _writing; });
    }
    /// Allow the writer and all later writes to complete.
    void release() {
        const auto lock = std::scoped_lock{_mutex};
        _released = true;
        _condition.notify_all();
    }

private:
    mutable std::mutex _mutex;          ///< Protects the writer state.
    std::condition_variable _condition; ///< Signals state changes.
    bool _writing{};                    ///< Whether the first write started.
    bool _released{};                   ///< Whether writes may complete.
};

/// A demo writer that simulates a failed destination.
/// @notest{This support type is exercised by the compiled statistics demo.}
class FailingLogWriter final : public el::log::LogWriter {
public: // implement LogWriter
    void write(
        [[maybe_unused]] const el::log::LogEntryConstPtr &entry,
        [[maybe_unused]] const el::log::LogLineConstPtr &line) override {
        throw std::runtime_error{"Simulated log destination failure."};
    }
};

/// A demo writer that retains the most recently delivered entry.
/// @notest{This support type is exercised by the compiled message-size demo.}
class CapturingLogWriter final : public el::log::LogWriter {
public: // implement LogWriter
    void write(const el::log::LogEntryConstPtr &entry, [[maybe_unused]] const el::log::LogLineConstPtr &line) override {
        const auto lock = std::scoped_lock{_mutex};
        _entry = entry;
    }

public:
    /// Get the most recently delivered entry.
    /// @return The captured immutable entry, or null before delivery.
    [[nodiscard]] auto lastEntry() const -> el::log::LogEntryConstPtr {
        const auto lock = std::scoped_lock{_mutex};
        return _entry;
    }

private:
    mutable std::mutex _mutex;        ///< Protects the captured entry.
    el::log::LogEntryConstPtr _entry; ///< Most recently delivered entry.
};

}
