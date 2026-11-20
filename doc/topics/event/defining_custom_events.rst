..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Defining and Posting Typed Events

*********************************
Defining and Posting Typed Events
*********************************

Suppose two subtitles are visible and one expires.
What should the playback engine send to the display? “Clear the screen” would erase both.
“Cue 1 on the English track ended” gives the display enough information to remove one caption and keep the other.
That distinction is a useful starting point for designing an event: describe the change that took place, including the
identity of the thing that changed.
Leave presentation to the recipient.

Start with the Consumer
=======================

Your application can expose a callback taking an ordinary C++ value or a reference to it.
This is the contract the consumer programs against.
Below it, a backend can translate queued events into calls to that callback.
A second consumer, such as a transcript writer, can use the same contract without decoding anything from the queue.

.. erbsland-in-practice-begin:: Taking One Caption Off Screen

In :doc:`/demo_apps/event/caption_player` the application connects its display while starting the engine:

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionPlayerApp.cpp
    :show: a:captions(+0,+4)
    :sha256: 8df8a7cfdaabc2a6cc8d85d52859708a2a0bb0631066f7ff95e98e60dac7bac4

.. code-block:: cpp

    auto captions = std::make_unique<CaptionBackend>(protocol, _tracks);
    captions->start(
        el::TimePoint::now(),
        [this](const CueBoundary &boundary) -> void { displayCue(boundary); },
        [this]() -> void { quit(); });

.. erbsland-demo-end::

At 1,200 ms the first English cue ends.
A second English cue is already on screen.
Both belong to one track, so even the filename alone would be too coarse a key.
The display uses a pair of positions:

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionPlayerApp.cpp
    :show: a:key(+0,+5)
    :sha256: 646e39c32e1d2ffdc67013e1d8c38d2662dae498d76709a73056863851acac30

.. code-block:: cpp

    const auto key = std::pair{boundary.track, boundary.cue};
    if (boundary.entering) {
        _active.insert(key);
    } else {
        _active.erase(key);
    }

.. erbsland-demo-end::

The display borrows the boundary during the call and stores the key by value.
The track still owns the caption text.

.. erbsland-in-practice-end::

Give the Protocol an Identity
=============================

An :cpp:class:`Event <erbsland::event::Event>` carries an :cpp:class:`EventId <erbsland::event::EventId>` and can own a
payload derived from :cpp:class:`EventData <erbsland::event::EventData>`.
Choose one identifier for changes with the same meaning and payload layout.
Individual occurrences can then differ in their data.
Register a reverse-domain name through ``Application::eventRegistry()`` during setup and share the returned ID with
producers and receivers.
Duplicate registration of a name is an error.
Save the name if you need a durable identifier outside this run.

The backend has a separate ``EventBackendId`` identifying the service installed on a particular loop.
Registering either name assigns an identity.
Delivery also requires an installed backend that recognizes the event.

.. erbsland-in-practice-begin:: One Event Type for Every Cue

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionProtocol.hpp
    :show: c:@return Identifiers(-1,+7)
    :sha256: 715a7c0e55ba26c8c20748b05f9323f560fc4962e4d03259e3209bde46ec761f

.. code-block:: cpp

    static auto registerWith(el::EventRegistry &registry) -> CaptionProtocol {
        using namespace el::text::literals;
        return {
            registry.registerBackend("dev.erbsland.demo.CaptionPlayback"_el),
            registry.registerEvent("dev.erbsland.demo.CueBoundary"_el),
        };
    }

.. erbsland-demo-end::

The player calls this once.
Adding another subtitle file creates more cue boundaries under the same ID.
The payload is short enough to own by value:

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CueEventData.hpp
    :show: c:The queue owns(+0,+13)
    :sha256: eccf4dfe66d63048b8695c38e76a0761b076e930d26e955fefb5b518fc5a00a8

.. code-block:: cpp

    /// The queue owns this copy of the boundary until dispatch returns.
    class CueEventData final : public el::EventData {
    public:
        /// Wrap a cue boundary for delivery through the event loop.
        /// @param value The transition to deliver.
        explicit CueEventData(CueBoundary value) : _boundary{value} {}

    public: // accessors
        /// The boundary borrowed by the display during dispatch.
        [[nodiscard]] auto boundary() const noexcept -> const CueBoundary & { return _boundary; }

    private:
        CueBoundary _boundary; ///< Owned transition, independent of the timeline cursor.
    };

.. erbsland-demo-end::

The wrapped value contains four fields:

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CueBoundary.hpp
    :show: c:One change(+0,+6)
    :sha256: 59aae4c51454a5f29e90b05f5564f92ae288d4e19c9327a0cfd2565527203e71

.. code-block:: cpp

    /// One change to the set of visible captions.
    struct CueBoundary {
        el::TimeDelta offset; ///< Time on the subtitle timeline.
        bool entering;        ///< True when the cue appears, false when it disappears.
        std::size_t track;    ///< Position of the file in command-line order.
        std::size_t cue;      ///< Position of the cue in that file.
    };

.. erbsland-demo-end::

The indices still identify the same captions at delivery because the application loads all tracks before playback and
keeps them in their original order.
A player that allows live editing would need identities that survive edits.

.. erbsland-in-practice-end::

Move the Data into the Queue
============================

Construct the payload while the input is available, then transfer it into the event using a unique pointer.
Application producers can pass the event to ``EventTarget::post()``.
A backend uses the target supplied to ``attach()`` and can check the Boolean result of ``postFromBackend()``.
Acceptance transfers responsibility for dispatch to the loop.
The producer can advance its own state before a consumer runs.

.. erbsland-in-practice-begin:: Posting the End of the First English Cue

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionBackend.cpp
    :show: f:CaptionBackend::poll
    :sha256: b9c2916d80067d6bfe43f7c9555a4509afdbb31fc5a05b1b4d4121275a98145b

.. code-block:: cpp

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

At the end of “The tide is turning.” the copied boundary carries offset 1,200 ms, track zero, cue zero, and
``entering == false``.
The next iteration can queue the commentary change immediately.
Each queued event still has its own boundary when the loop comes back to dispatch it.
``Event::time()`` records when the wrapper was constructed.
The transcript takes its subtitle timestamp from the payload.

.. erbsland-in-practice-end::

Claim the Event During Dispatch
===============================

The loop offers a custom event to its installed backends until one returns ``true``.
Return ``false`` for an unrelated ID so the next backend gets a turn.
Once the ID matches, check the payload before accessing its fields.
A matching ID with missing or unexpected data indicates a disagreement between the producer and receiver.
If every backend declines, the event leaves the queue without a recipient.

.. erbsland-in-practice-begin:: From Queued Data to the Display

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

Only this method needs to know that a cue callback arrived through an ``Event``.
The reference passed to the display lives through the call.
A consumer that wants to process the boundary later must take its own copy.

.. erbsland-in-practice-end::

When the producer already knows the operation to run, ``invoke()`` can carry its arguments in a captured callback.
For one future call, ``invokeAfter()`` also supplies the timing.
Defining a custom event earns its extra setup when you have a domain with its own input and dispatch rules and want to
carry its changes through the loop as data.
:doc:`writing_event_backends` shows how to connect such a domain's deadlines to the loop.
