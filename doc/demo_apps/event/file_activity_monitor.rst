..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*********************************************************
FileActivityMonitor: Watching for Activity and Inactivity
*********************************************************

``FileActivityMonitor`` watches an existing regular file and reports changes to its size or modification time.
You can use it to observe a log or another file being updated by a separate process, then leave monitoring automatically
after a quiet period or an overall time limit.
Its three owned timers demonstrate fixed-rate polling, a restartable one-shot timeout, and an independent deadline.
See :doc:`/topics/event/scheduling_work` for the general scheduling and lifetime rules.

Choosing the File and Time Limits
=================================

The required argument is a file path.
``--interval-ms`` selects the positive polling interval, defaulting to 250 ms.
``--idle-timeout`` selects the positive quiet period in seconds, defaulting to five seconds.
``--timeout`` supplies a nonnegative overall limit in seconds; its default of zero leaves that limit disabled.
The quiet period remains enabled, so an unchanged file always leads to completion.

Command-line handling validates these values and resolves the path to an existing regular file before queuing
``begin()`` on the main loop.
The application object lives around ``run()``, covering all dispatch and cleanup:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/main.cpp
    :show: f:main
    :sha256: 9fa98783d9276523110828a4752319623862353c7b7b4e3f8b22801bcbbd747b

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = demo::FileActivityMonitorApp{argc, argv};
        return app.run();
    }

.. erbsland-demo-end::

No worker thread is needed for this small metadata query.
All mutable monitoring state and timer callbacks stay on the main loop.
The application retains its snapshots, configuration, and timers as members:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.hpp
    :show: c:Reloaded on each poll-c:Starts once and is never extended
    :sha256: 1371c0b61485408818d064e176696f6a1c843bb637eb825714d64f2a6c5596d9

.. code-block:: cpp

    el::PathInfo _fileInfo;                         ///< Reloaded on each poll; never reads file contents.
    el::ByteCount _previousSize;                    ///< Size in the last observed snapshot.
    el::Timestamp _previousModification;            ///< Modification time in the last observed snapshot.
    el::TimeDelta _interval{el::Milliseconds{250}}; ///< Positive polling interval.
    el::TimeDelta _idleTimeout{el::Seconds{5}};     ///< Quiet period measured from the latest observation.
    el::TimeDelta _timeout{el::Seconds{0}};         ///< Optional overall limit; zero disables it.
    el::EventTimerPtr _pollTimer;                   ///< Repeats on a fixed cadence.
    el::EventTimerPtr _idleTimer;                   ///< Restarts after each observed change.
    el::EventTimerPtr _deadline;                    ///< Starts once and is never extended by activity.

.. erbsland-demo-end::

Starting Three Independent Schedules
====================================

``begin()`` first calls ``poll()`` to establish a baseline.
At this point the inactivity timer does not exist, so the initial metadata is recorded without reporting activity.
It then starts the inactivity timer, the repeating polling timer, and the optional overall deadline:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.cpp
    :show: f:FileActivityMonitorApp::begin
    :sha256: 43d779ed3311fd8170669a9c49ad2ede5c21d45b678ed325b7bd1f14afea592d

.. code-block:: cpp

    void FileActivityMonitorApp::begin() {
        // Establish the initial snapshot before starting the quiet period.
        poll();
        _idleTimer = loop()->createTimer([this]() -> void { finish("Inactivity timeout reached."_el); });
        _idleTimer->startOnce(_idleTimeout);
        _pollTimer = loop()->createTimer([this]() -> void { poll(); });
        _pollTimer->startFixedRate(_interval);
        if (_timeout.isPositive()) {
            _deadline = loop()->createTimer([this]() -> void { finish("Overall deadline reached."_el); });
            _deadline->startOnce(_timeout);
        }
        el::stdOut()->printLine("Monitoring file: "_el, _fileInfo.path());
        el::stdOut()->flush();
    }

.. erbsland-demo-end::

The polling timer follows the configured cadence without accumulating missed ticks.
Each one-shot callback forwards to ``finish()`` with a reason for stopping.
These lambdas borrow the application through ``this``; its lifetime surrounds main-loop execution, and cleanup stops and
releases every timer before the application is destroyed.
The initial message is flushed so another process can wait until monitoring is ready before changing the file.

Detecting Changes and Restarting the Quiet Period
=================================================

Each poll explicitly reloads the file type, size, and timestamps.
A disappeared or nonregular file ends monitoring with an application error.
Otherwise, the current size and modification time are compared with the previous snapshot:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.cpp
    :show: f:FileActivityMonitorApp::poll, f:FileActivityMonitorApp::onActivity
    :sha256: 157dfd9066878014afb5e3a32b09dab97571b5b43398af29446834e92b3688c5

.. code-block:: cpp

    void FileActivityMonitorApp::poll() {
        _fileInfo.reload(el::PathInfoParts{el::PathInfoPart::Type, el::PathInfoPart::Size, el::PathInfoPart::Times});
        if (!_fileInfo.isRegularFile()) {
            throw el::ApplicationError{"The monitored file is unavailable or no longer a regular file."_el};
        }
        const auto size = _fileInfo.fileSize();
        const auto modification = _fileInfo.lastModified();
        const auto changed = size != _previousSize || modification != _previousModification;
        _previousSize = size;
        _previousModification = modification;
        // The initial snapshot has no activity timer yet.
        if (changed && _idleTimer) {
            onActivity();
        }
    }

    void FileActivityMonitorApp::onActivity() {
        // Only an observed change restarts the quiet period; the overall deadline stays unchanged.
        _idleTimer->startOnce(_idleTimeout);
        el::stdOut()->printLine("Changed: "_el, _previousSize, " bytes; modified "_el, _previousModification.toString());
        el::stdOut()->flush();
    }

.. erbsland-demo-end::

An unchanged poll leaves the inactivity timer alone.
An observed change restarts that timer and prints the new metadata, including when only the modification time changed.
The overall deadline is never restarted.
Continuous activity can therefore postpone the quiet-period exit but cannot extend the overall time budget.

This is a metadata monitor, not a file-completion protocol.
It can miss changes between polls, and an unchanged size and timestamp do not prove that a writer has finished.
The quiet period starts at the last **observed** change, rather than at the exact time of a filesystem operation.
Choose a polling interval shorter than the changes you want to observe and the quiet period you want to measure.
The path is resolved at startup; changes to a symbolic link itself are not followed to a new target.
For a filesystem where metadata reads can block noticeably, move those reads to a worker and return observations to the
main loop.

Observing a Real File
=====================

From the repository root in a POSIX shell, create a disposable log and start a bounded session:

.. code-block:: console

    $ activity_dir=$(mktemp -d)
    $ printf 'Started\n' > "$activity_dir/activity.log"
    $ cmake-build-debug/demo-apps/event/file_activity_monitor "$activity_dir/activity.log" --idle-timeout 10 --timeout 30

In another terminal, append a line to the absolute file path printed by the monitor before its quiet period expires.
For example, replacing the path below with the displayed path:

.. code-block:: console

    $ printf 'Job completed\n' >> /path/from/monitor/activity.log

The next poll reports ``Changed: ...`` and starts a new ten-second quiet period.
Further writes keep extending that quiet period; the thirty-second overall deadline stays fixed.
The monitor only reads metadata and never writes to the monitored file.

The following captured commands use disposable, unchanged files so both completion reasons can be observed without
another terminal.
The first exits after one quiet second; the second reaches its one-second overall limit before its five-second quiet
period:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.cpp
    :show: f:FileActivityMonitorApp::finish
    :exec: event/file_activity_monitor {file:text} --idle-timeout 1 --timeout 5
    :exec-2: event/file_activity_monitor {file:text} --idle-timeout 5 --timeout 1
    :sha256: acbfe9ed7e3371307063277d225c1369bf9b0afc29913dee5ebbd051fa4cfbce

.. code-block:: cpp

    void FileActivityMonitorApp::finish(const el::String &reason) {
        stopTimers();
        el::stdOut()->printLine(reason);
        el::stdOut()->flush();
        quit();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/file_activity_monitor sample-text.txt --idle-timeout 1 --timeout 5

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMonitoring file: sample-text.txt
        Inactivity timeout reached.␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ event/file_activity_monitor sample-text.txt --idle-timeout 5 --timeout 1

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMonitoring file: sample-text.txt
        Overall deadline reached.␛[0m

.. erbsland-demo-end::

Stopping and Handling Failures
==============================

``finish()`` stops every timer before requesting application shutdown.
Stopping the currently executing one-shot callback is safe: it cancels scheduling without interrupting that callback.
``cleanup()`` repeats cancellation and releases the handles, including after an error bypasses ``finish()``:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.cpp
    :show: f:FileActivityMonitorApp::stopTimers, f:FileActivityMonitorApp::cleanup
    :sha256: 5f5df5e725b4071cdf1f064988e05ae5ebc676ae270e8e8834941a1e1d6c0d50

.. code-block:: cpp

    void FileActivityMonitorApp::stopTimers() noexcept {
        for (const auto &timer : {_pollTimer, _idleTimer, _deadline}) {
            if (timer) {
                timer->stop();
            }
        }
    }

    void FileActivityMonitorApp::cleanup() noexcept {
        stopTimers();
        _pollTimer.reset();
        _idleTimer.reset();
        _deadline.reset();
    }

.. erbsland-demo-end::

Either timeout is a successful completion.
Invalid options, an unsuitable initial path, or a file that becomes unavailable or nonregular produce a failure exit and
an application diagnostic.
An exception from a polling callback follows the application's failure lifecycle, so cleanup does not depend on another
timer firing.
