..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Creating Event Sources

****************************************
Creating Event Sources and Notifications
****************************************

A reusable component often knows when something has happened without knowing what its caller wants to do about it.
An event source lets the component publish that observation and the caller supply the response.
Consumers retain the source, install handlers through its editor, and let its owner loop deliver notifications.
This page builds on :doc:`using_event_apis` and :doc:`callback_lifetimes`.

Start with the Promise to the Consumer
======================================

Give each notification a meaning that the implementation can observe.
Decide where the handler executes, how long its arguments remain valid, and what happens if the consumer changes its
handler during delivery.
A single replaceable handler is a useful interface when the operation has one consumer.
Give that slot an explicit name and a way to clear it.

.. erbsland-in-practice-begin:: Receiving an inbox arrival

The :doc:`inbox monitor </demo_apps/event/inbox_monitor>` reports a regular file when its name first appears in a
directory scan.
The first scan reports the files already present.
A name can be reported again if a later scan observes its absence and a subsequent scan finds it again.
Files that appear and disappear between scans can go unseen.
An arrival says nothing about whether the writer has finished.

.. erbsland-demo::
    :source: event/apps/InboxMonitor/src/InboxMonitorApp.cpp
    :show: f:InboxMonitorApp::watch
    :sha256: b262e676d09fb72fb46baadbfee81869f5f5c7f64a34b649b0673a7dc7148c1f

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

.. erbsland-demo-end::

The CLI installs the handler before starting because the first scan delivers synchronously.
The handler borrows the path for the duration of the call.
A consumer that needs it later can copy the path.
The application member keeps the source alive between scans, including the idle interval when no callback is executing.

.. erbsland-in-practice-end::

Give the Editor the Source's Lifetime
=====================================

Derive the source from :cpp:class:`EventSource <erbsland::event::EventSource>` and return a typed editor by reference
from ``events()``.
That lets callers name domain-specific notifications while generic code can still use
:cpp:class:`EventEditor <erbsland::event::EventEditor>`.
Keep the editor at a stable address for the lifetime of the source.
A reference to that editor borrows the source.
A caller that needs ownership can obtain it explicitly with ``source()``.

Create a loop-affine source while its loop is dispatching.
Enforce that same ownership wherever the source or editor exposes mutable state.
A caller on another thread can post a command to the owner target.
The queued command needs its own safe reference to the source.

.. erbsland-in-practice-begin:: Editing a handler on the inbox loop

.. erbsland-demo::
    :source: event/apps/InboxMonitor/src/InboxWatcher.cpp
    :show: f:InboxWatcher::events
    :sha256: 5749395f57089e1c5c30c84f39f0a65e8bcc44a4bfb954842a9cd86914b65fac

.. code-block:: cpp

    auto InboxWatcher::events() -> InboxEvents & {
        verifyCurrentOwnerLoop();
        return _events;
    }

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/apps/InboxMonitor/src/InboxEvents.cpp
    :show: f:InboxEvents::source, f:InboxEvents::target, f:InboxEvents::onFileArrived
    :sha256: 5ac7d2e28bbbdd07643c937ee6b4f2f9fb9f4e1b99e3a69cf50a202026ad2c4d

.. code-block:: cpp

    auto InboxEvents::source() const -> el::EventSourcePtr {
        return _watcher.shared_from_this();
    }

    auto InboxEvents::target() const noexcept -> el::EventTargetPtr {
        return _watcher.ownerLoop();
    }

    auto InboxEvents::onFileArrived(Arrival handler) -> InboxEvents & {
        _watcher.verifyCurrentOwnerLoop();
        _arrival.reset();
        if (handler) {
            _arrival = std::make_shared<Arrival>(std::move(handler));
        }
        return *this;
    }

.. erbsland-demo-end::

The editor is a data member with a reference back to its enclosing watcher.
Only ``source()`` creates a strong reference for the caller.
The setter checks loop ownership even if a caller kept the editor reference from an earlier callback.
Passing an empty function clears the slot.
If a handler itself retains the watcher, clear that slot to break the ownership cycle.

.. erbsland-in-practice-end::

Commit the Observation Before Calling Out
=========================================

Treat a call to a handler as a transfer of control.
The consumer can stop the operation, replace the handler that is currently running, or release its last source pointer
before the call returns.
Commit the source state needed by such calls first.
Keep the active handler alive through its return, then consult the current registration before delivering another item.

.. erbsland-in-practice-begin:: Stopping halfway through a batch

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

The walk finishes collecting paths before it invokes any arrival handler.
This synchronous scan suits a small, local inbox.
If enumeration can stall or the directory is large, collect a snapshot on a worker and deliver it to the owner loop
before updating the source state.
Suppose the scan discovers three files.
A handler for the first can install a different handler for the second.
The local handler pointer keeps the first callable, including its captures, alive until it returns.
If the second handler calls ``stop()``, the loop over arrivals ends before the third file.

.. erbsland-demo::
    :source: event/apps/InboxMonitor/src/InboxWatcher.cpp
    :show: f:InboxWatcher::start
    :sha256: 06c4fe0cbd6242324677f78c8a180373e13f3d0a286fb70e7dc6d1289866d117

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

.. erbsland-demo-end::

The first synchronous scan holds a strong local pointer.
On later scans the timer locks a weak pointer for just one invocation.
In both cases, releasing the last external owner during a handler leaves the source alive until the active scan returns.
Call ``stop()`` when remaining deliveries in that scan must also cease.
Errors from the walk or the handler propagate to the enclosing dispatch error policy, described in
:doc:`handling_event_errors`.

.. erbsland-in-practice-end::

A component can publish its own notifications using an existing timer or callback target.
When the underlying operation needs new raw events that the loop must route, continue with
:doc:`defining_custom_events`.
