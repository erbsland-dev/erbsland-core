..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Writing an Event Backend

************************
Writing an Event Backend
************************

A backend gives the event loop access to work it would otherwise know nothing about.
Consider a playback engine that already has a timeline.
It can answer two questions cheaply: “When is the next change?” and “Which changes are due now?” If it answers them
directly, the loop can sleep through a quiet interval and wake in time for the next change.
The engine keeps one schedule even when many tracks are playing.

If your component just needs a repeating callback or a delay before one operation, start with
:doc:`scheduling_work`. A backend is worth considering when a subsystem already owns the information needed to
schedule and interpret a whole family of events.

Install the Backend Where the Loop Is Owned
===========================================

Configure the domain and connect its application callbacks before entering dispatch.
Move the backend into :cpp:func:`EventLoop::registerBackend() <erbsland::event::EventLoop::registerBackend>` while the
loop is stopped.
Its ID must be valid and unique among the backends installed on that loop.
The loop takes ownership and supplies a weak target through ``attach()``.
The backend can lock that target when it needs to queue work without keeping the loop alive.

.. erbsland-in-practice-begin:: Starting Caption Playback

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionPlayerApp.cpp
    :show: f:CaptionPlayerApp::installCaptionBackend
    :sha256: 08e165a9ef8d157a0152021e4c4b4610addadd5a9bddbcd26e5a5594eae88d12

.. code-block:: cpp

    void CaptionPlayerApp::installCaptionBackend() {
        const auto protocol = CaptionProtocol::registerWith(eventRegistry());
        auto captions = std::make_unique<CaptionBackend>(protocol, _tracks);
        captions->start(
            el::TimePoint::now(),
            [this](const CueBoundary &boundary) -> void { displayCue(boundary); },
            [this]() -> void { quit(); });
        mainEventLoop().registerBackend(std::move(captions));
    }

.. erbsland-demo-end::

The player calls this method at the end of command-line handling, after loading the tracks.
It sets time zero, connects the callbacks, and installs the backend through the derived application's protected
``mainEventLoop()`` accessor.
On return from startup, the default ``Application::main()`` enters that same loop.
Playback and any other main-loop sources now share a thread and a wait driver.
See :doc:`using_event_loops` for the surrounding lifecycle.

.. erbsland-in-practice-end::

Tell the Loop How Long It Can Wait
==================================

Return an absolute monotonic time from ``nextWakeTime()`` when you have a deadline.
Return an empty optional when no timed work remains.
The loop considers deadlines from all its backends and can also wake for posted work.
An early wake is ordinary: the next poll must still compare the deadline with the supplied time.

.. erbsland-in-practice-begin:: The Quiet Interval Between Subtitles

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionBackend.cpp
    :show: f:CaptionBackend::nextWakeTime
    :sha256: e21a8fd0a0788e315ee95d607f41a778c01750bec3a50ad3a163900f392ab239

.. code-block:: cpp

    auto CaptionBackend::nextWakeTime() const -> std::optional<el::TimePoint> {
        if (_next == _timeline.size()) {
            return {};
        }
        return _start + _timeline[_next].offset;
    }

.. erbsland-demo-end::

Once the opening caption has been queued, commentary first appears at 400 ms.
The backend can report exactly that deadline.
Adding the offset to the original start also keeps the 1,200 ms cue end in place if dispatch of an earlier cue ran late.

.. erbsland-in-practice-end::

Turn Due Work into Events
=========================

In ``poll(now)``, examine the domain state, post the due events, and advance past work that was accepted.
The loop calls the backend on the dispatch thread.
Sleeping inside this hook would keep other backends, callbacks, and the loop's own wait driver from making progress.
Prepare inputs that could block, such as file reads, before dispatch or outside this thread.

.. erbsland-in-practice-begin:: Three Boundaries on One Loop Turn

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionBackend.cpp
    :show: f:CaptionBackend::attach, f:CaptionBackend::poll
    :sha256: 3d6872746c046418d61bfb0f8619c527964e2f7555a16b46cf1d161662ac9a88

.. code-block:: cpp

    void CaptionBackend::attach(el::EventBackendTargetWeakPtr target, el::EventLoopDriverWeakPtr) {
        _target = std::move(target);
    }

    void CaptionBackend::poll(const el::TimePoint now) {
        while (_next < _timeline.size() && _start + _timeline[_next].offset <= now) {
            const auto target = _target.lock();
            auto event = el::Event{_protocol.cueBoundary(), std::make_unique<CueEventData>(_timeline[_next])};
            if (!target || !target->postFromBackend(std::move(event))) {
                throw el::ApplicationError{"Playback stopped because a caption could not be queued."_el};
            }
            ++_next;
        }
    }

.. erbsland-demo-end::

At 1,200 ms the first English cue ends, one commentary cue ends, and the next commentary cue starts.
The ordered timeline places ends before starts at a shared offset, then uses file and cue order to break ties.
A late poll can discover all three at once.
The backend queues each boundary once and lets normal loop dispatch call the display.
Calling the display directly inside the polling hook would run application code partway through advancing the timeline
and bypass the dispatch error handler.

Every boundary is required to reconstruct the visible captions.
If the queue refuses one, continuing would risk leaving stale text on screen or waiting for a final event that can never
arrive.
The backend throws from ``poll()``, which propagates directly out of the loop to the application's error handling.

.. erbsland-in-practice-end::

A fixed timeline needs no further wake requests.
If another thread can introduce an earlier deadline while the loop sleeps, synchronize the change and call
``wakeFromBackend()`` after making it visible.
The wake prompts the loop to poll and recompute its wait.
Posting an accepted event already wakes the loop.

Finish After Delivery
=====================

Running out of input tells you that the backend has finished producing.
Events it just queued may still be waiting for dispatch.
Signal completion at the point promised by the application's interface.
For a display callback, that can be after it returns from processing the final change.

.. erbsland-in-practice-begin:: Clearing the Last Caption

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionBackend.cpp
    :show: f:CaptionBackend::handleEvent
    :sha256: cf09268fdc2e5ccec88831180e3e591e58a76d046b577c9a9b45b935792dc366

.. code-block:: cpp

    auto CaptionBackend::handleEvent(const el::Event &event) -> bool {
        if (event.identifier() != _protocol.cueBoundary()) {
            return false;
        }
        const auto *data = dynamic_cast<const CueEventData *>(event.data().get());
        if (!data) {
            throw el::ApplicationError{"A caption event is missing its cue boundary."_el};
        }
        _onCue(data->boundary());
        ++_delivered;
        if (_delivered == _timeline.size()) {
            _onFinished();
        }
        return true;
    }

.. erbsland-demo-end::

After the final end event reaches the display at 2,400 ms, the completion callback requests quit.
The application has now observed every accepted boundary.
If a display callback throws a Core runtime error, the main loop's application-installed handler stops dispatch and the
application reports the failure during shutdown.

Follow the full session in :doc:`/demo_apps/event/caption_player`.
:doc:`defining_custom_events` explains the ID and payload checks at the start of the handler.

.. erbsland-in-practice-end::

For the error boundaries of individual hooks, see :doc:`handling_event_errors`.
If your backend must operate under a bounded queue, choose what each rejected post means for your domain using
:doc:`controlling_event_load`.
When your input also needs a custom native wait, see :doc:`integrating_event_loop_drivers`.
