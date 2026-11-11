// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TimeAmounts.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/Timestamp.hpp>

namespace demo {

/// Report observed file activity until a quiet period or an overall deadline ends monitoring.
/// The application owns all timers; their callbacks and mutable state stay on the main loop.
class FileActivityMonitorApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;
    void cleanup() noexcept override;

private:
    void begin();
    void poll();
    void onActivity();
    void finish(const el::String &reason);
    void stopTimers() noexcept;

private:
    el::PathInfo _fileInfo;                         ///< Reloaded on each poll; never reads file contents.
    el::ByteCount _previousSize;                    ///< Size in the last observed snapshot.
    el::Timestamp _previousModification;            ///< Modification time in the last observed snapshot.
    el::TimeDelta _interval{el::Milliseconds{250}}; ///< Positive polling interval.
    el::TimeDelta _idleTimeout{el::Seconds{5}};     ///< Quiet period measured from the latest observation.
    el::TimeDelta _timeout{el::Seconds{0}};         ///< Optional overall limit; zero disables it.
    el::EventTimerPtr _pollTimer;                   ///< Repeats on a fixed cadence.
    el::EventTimerPtr _idleTimer;                   ///< Restarts after each observed change.
    el::EventTimerPtr _deadline;                    ///< Starts once and is never extended by activity.
};

}
