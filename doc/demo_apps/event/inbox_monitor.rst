..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Demo Applications; InboxMonitor

*******************************************
InboxMonitor and File Arrival Notifications
*******************************************

Point InboxMonitor at a directory where another process deposits order files.
At startup it prints the names of the regular files already there.
It then reports new arrivals approximately once per second.
A consumer can replace the printing code with its own handler while keeping the same watcher.

Try a Delivery
==============

Build the ``inbox-monitor`` target.
From the repository root, try the bundled two-file inbox:

.. code-block:: console

    $ cmake-build-debug/demo-apps/event/inbox-monitor --once demos/event/apps/InboxMonitor/example-files
    Arrived  orders-001.txt
    Arrived  orders-002.txt
    Scan complete. Arrivals: 2

To watch a live delivery, omit ``--once`` and point the application at your own directory.
Copy a file into it from a second terminal.
The already visible names stay quiet.
End the monitor with Ctrl-C.
For a finite inventory, an empty inbox is also a successful result:

.. erbsland-demo::
    :source: event/apps/InboxMonitor/src/InboxMonitorApp.cpp
    :show: f:InboxMonitorApp::watch
    :exec: event/inbox-monitor --once {directory:inbox}
    :show-cmd-line:
    :sha256: 63a5d1969e51ff8986fa5814688148ec5f3f7734c969020b405f4ffc06e1bd24

.. code-block:: cpp

    void InboxMonitorApp::watch(const el::Path &directory, const bool once) {
        _watcher = InboxWatcher::create(directory);
        _watcher->events().onFileArrived([this](const el::Path &file) -> void {
            ++_arrivals;
            el::io::printLine("Arrived  "_el, file.name());
            el::stdOut()->flush();
        });
        _watcher->start();
        if (once) {
            _watcher->stop();
            el::io::printLine("Scan complete. Arrivals: "_el, _arrivals);
            quit();
        }
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/inbox-monitor --once inbox

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mScan complete. Arrivals: 0␛[0m

.. erbsland-demo-end::

From Option Handling to the First Scan
======================================

.. erbsland-demo::
    :source: event/apps/InboxMonitor/src/InboxMonitorApp.cpp
    :show: f:InboxMonitorApp::handleCommandLineOptions
    :sha256: 2bea64bda59cd9deea8208ff5807a334c6866c7910048ea32dd1654cca188b33

.. code-block:: cpp

    void InboxMonitorApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
        const auto directory = el::Path::fromNativeOrThrow(values->getText(cDirectory)).toAbsoluteOrThrow();
        if (!directory.info().isDirectory()) {
            throw el::OptionError{"Expected an existing inbox directory."_el, values->value(cDirectory)};
        }
        const auto once = values->getFlag(cOnce);
        loop()->invoke([this, directory, once]() -> void { watch(directory, once); });
    }

.. erbsland-demo-end::

The application validates the directory before it starts work.
It queues the setup so that ``InboxWatcher::create()`` can bind the source to the current loop.
The application owns the watcher; the watcher's editor owns the handler.
The first scan runs inside ``start()``, so the count is complete as soon as that method returns in one-scan mode.

What Counts as an Arrival
=========================

.. erbsland-demo::
    :source: event/apps/InboxMonitor/src/InboxWatcher.cpp
    :show: f:InboxWatcher::scan
    :sha256: 787d65fdbe92b2b9b7dc09c4e37e61d8fb81d90d971fe61c51ff4e6a9160549c

.. code-block:: cpp

    void InboxWatcher::scan() {
        auto visible = el::StringSet{};
        auto arrivals = el::PathList{};
        auto options = el::PathWalkOptions{};
        options.setTypes(el::PathType::RegularFile).setSymlinkMode(el::SymlinkMode::Skip);
        options.setDescendFilter([this](const el::Path &path, el::PathType) -> bool { return path == _directory; });
        _directory.walker().walkOrThrow(
            [&](const el::Path &path) -> el::PathWalkStatus {
                visible.insert(path.name());
                if (!_known.contains(path.name())) {
                    arrivals.append(path);
                }
                return el::PathWalkStatus::Continue;
            },
            options);
        _known = std::move(visible);
        for (const auto &path : arrivals) {
            if (!_timer->isActive()) {
                break;
            }
            if (const auto handler = _events._arrival) {
                (*handler)(path);
            }
        }
    }

.. erbsland-demo-end::

Each scan considers regular files immediately inside the directory.
It skips links and subdirectories.
The walker's deterministic order also orders the notifications.
Comparing names with the previous scan means that an edit to an existing file stays quiet.
If a producer wants arrivals to mean “ready for import”, it can finish writing outside the inbox and move the completed
file into it.

The known-name set changes only after a successful walk.
The loop then checks the watch state and obtains the current handler separately for each path.
Stopping in a handler ends the current batch.
A filesystem or callback error propagates through the application's main loop and causes application failure.

Who Keeps the Timer Alive
=========================

.. erbsland-demo::
    :source: event/apps/InboxMonitor/src/InboxWatcher.cpp
    :show: f:InboxWatcher::start, f:InboxWatcher::stop
    :sha256: 1c352882d9b8c4147edded11512e24054c12e0728f8f8bfaf49aac0645aa1a17

.. code-block:: cpp

    void InboxWatcher::start() {
        verifyCurrentOwnerLoop();
        if (_timer) {
            throw el::LogicFailure{"An inbox watch can only start once."_el};
        }
        const auto self = std::static_pointer_cast<InboxWatcher>(shared_from_this());
        _timer = ownerLoop()->createTimer([weak = std::weak_ptr{self}]() -> void {
            if (const auto watcher = weak.lock()) {
                watcher->scan();
            }
        });
        _timer->startFixedDelay(el::Seconds{1});
        self->scan();
    }

    void InboxWatcher::stop() {
        verifyCurrentOwnerLoop();
        if (_timer) {
            _timer->stop();
        }
    }

.. erbsland-demo-end::

The watcher retains the timer.
The timer callback holds a weak watcher reference during the idle interval and locks it on entry.
A slow scan finishes before the next fixed delay begins.
Only the active scan needs a strong reference to the watcher; releasing the application's pointer cannot leave a later
timer invocation with a dangling recipient.

See :doc:`/topics/event/creating_event_sources` for the editor contract and the behavior when a handler changes a
registration during delivery.
