// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/cterm/BlockString.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/Diagnostic.hpp>
#include <erbsland/ElapsedTimer.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/LogEntry.hpp>
#include <erbsland/ManagedEventThread.hpp>
#include <tool/frame/TaskProgress.hpp>

#include <mutex>

namespace tool::pre_commit::impl {

/// Render an inline task list, with a timer updating only the active interactive row.
class TaskDisplay final {
public:
    /// Bind the application terminal and start its refresh timer when interactive.
    /// @param terminal Initialized output terminal.
    explicit TaskDisplay(el::cterm::TerminalPtr terminal);
    /// Stop and join refresh work before restoring output styling.
    ~TaskDisplay();

public: // task presentation
    /// Begin one task row and bind its progress counters.
    /// @param title Short task label.
    /// @param progress Shared work counters.
    void begin(el::String title, frame::TaskProgress progress);
    /// Replace the active row with a permanent completion row.
    /// @param success Whether the task passed.
    /// @param warnings Whether warnings were emitted.
    /// @param elapsed Total task duration including preparation.
    void finish(bool success, bool warnings, el::time::TimeDelta elapsed);
    /// Print one retained log entry below its completed task.
    /// @param entry Captured diagnostic or verbose output.
    void message(const el::LogEntry &entry);
    /// Render the original structured exception diagnostic.
    /// @param diagnostic Failure with source location and cause information.
    void diagnostic(const el::Diagnostic &diagnostic);
    /// Print the final aggregate summary.
    /// @param passed Number of successful tasks.
    /// @param failed Number of failed tasks.
    /// @param elapsed Total run time.
    void summary(std::size_t passed, std::size_t failed, el::time::TimeDelta elapsed);
    /// Refresh the active row; also usable with virtual terminals in tests.
    void refresh();

private:
    /// Compose phase counts or the indeterminate activity indicator.
    [[nodiscard]] auto progressText(const frame::TaskProgressSnapshot &snapshot, int width) -> el::String;
    /// Fit the title and right-aligned detail to the current terminal width.
    void writeTaskRow(const el::String &mark, const el::String &detail, el::cterm::Foreground foreground);
    /// Available columns for activity details, retaining space for the task title.
    [[nodiscard]] auto detailWidth() const -> int;
    /// Overwrite one row without wrapping; output synchronization must already be held.
    void writeRow(const el::cterm::BlockString &text, el::cterm::Foreground foreground);

private:
    el::cterm::TerminalPtr _terminal;         ///< Shared initialized output terminal.
    el::cterm::BlockStyle _originalStyle;     ///< Restored after reporting.
    bool _interactive{};                      ///< Whether cursor controls are available.
    bool _active{};                           ///< Whether a row is being refreshed.
    std::mutex _mutex;                        ///< Serializes timer refresh and task transitions.
    el::String _title;                        ///< Current task name.
    frame::TaskProgress _progress;            ///< Current worker counters.
    el::ElapsedTimer _elapsed;                ///< Monotonic task duration.
    std::size_t _tick{};                      ///< Indeterminate animation position.
    el::event::ManagedEventThreadPtr _thread; ///< Timer owner, present only for interactive output.
    el::event::EventTimerPtr _timer;          ///< Ten-Hz refresh timer.
};

}
