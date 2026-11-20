..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Demo Applications; CaptionPlayer

**************************************************
CaptionPlayer: Playing Overlapping Subtitle Tracks
**************************************************

Play a dialogue track alongside a sound description and both can be visible at once.
A single “current subtitle” variable cannot represent this.
CaptionPlayer reads ordinary SubRip files, merges their start and end times, and keeps each caption visible for its own
interval.
It runs entirely on the calling thread and exits when the last caption has been removed.

Watch Two Tracks Together
=========================

From the repository root, build the executable and run the bundled coastal scene:

.. code-block:: console

    $ cmake --build cmake-build-debug --target caption-player
    $ cd demos/event/apps/CaptionPlayer/example-files
    $ ../../../../../cmake-build-debug/demo-apps/event/caption-player english.srt commentary.srt

At 800 ms you should see three cues: the first English line, a second English cue spanning two lines, and a description
of the waves.
At 1,200 ms “The tide is turning.” disappears.
“Look beyond the rocks.” stays on screen.
Use ``--plain`` to follow those same changes on separate output lines.
The numbers are offsets read from the files:

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionPlayerApp.cpp
    :show: f:CaptionPlayerApp::printTransition
    :files: ../../../demos/event/apps/CaptionPlayer/example-files/english.srt ../../../demos/event/apps/CaptionPlayer/example-files/commentary.srt
    :exec: event/caption-player --plain english.srt commentary.srt
    :show-cmd-line:
    :sha256: 39f2bd0ca0022866f8846f9a22347d51ed23c963e39006c935c1a2378e20693e

.. code-block:: cpp

    void CaptionPlayerApp::printTransition(const CueBoundary &boundary) const {
        const auto &track = _tracks.at(boundary.track);
        const auto &cue = track.cues().at(boundary.cue);
        auto change = "end"_el;
        if (boundary.entering) {
            change = "start"_el;
        }
        el::io::printLine(
            el::StringFormat{"{} ms  {}  {} #{}"_el}.build(
                boundary.offset.toMilliseconds().toRawValue(), change, track.name(), cue.number));
        if (boundary.entering) {
            for (const auto &line : cue.lines) {
                el::io::printLine("    "_el, line);
            }
        }
        el::stdOut()->flush();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/caption-player --plain english.srt commentary.srt

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m0 ms  start  english.srt #1
            The tide is turning.
        400 ms  start  commentary.srt #1
            Waves against the pier.
        800 ms  start  english.srt #2
            Look beyond the rocks.
            There is the lighthouse.
        1200 ms  end  english.srt #1
        1200 ms  end  commentary.srt #1
        1200 ms  start  commentary.srt #2
            The foghorn sounds.
        2000 ms  end  english.srt #2
        2400 ms  end  commentary.srt #2␛[0m

.. erbsland-demo-end::

Try a track of your own by passing its path as another argument.
Use numbered SubRip blocks with a timing line of the form ``00:00:00,800 --> 00:00:02,000``, one or more text lines, and
a blank line between blocks.
The player displays text literally, including any subtitle markup.
An unreadable file or malformed cue ends startup before the first caption appears.
Cue syntax diagnostics identify the file, line and block.

Find the Code Behind a Cue
==========================

The production sources are in ``demos/event/apps/CaptionPlayer/src``.
These are the three places to start:

.. list-table::
    :header-rows: 1

    * - Where to look
      - What it owns and decides
    * - ``CaptionTrack``
      - Reads one file. Keeps the original labels, text lines, and time interval for each cue.
    * - ``CaptionBackend``
      - Orders the boundaries, reports the next deadline, queues due changes, and delivers the typed callbacks.
    * - ``CaptionPlayerApp``
      - Retains the tracks, installs the backend, runs the loop, and updates the display's set of visible cues.

The application parses every track while handling its command-line options.
Each cue gets a stable address in the domain model: its track position plus its position within that track.
File labels remain useful for the transcript, and two files can both contain a cue labelled “1”.

Connect the Display Before Starting the Clock
=============================================

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

At the end of command-line handling, the player calls this setup method.
Constructing the backend builds the ordered timeline from the loaded tracks.
Only then does the player take its monotonic start time.
Registration transfers ownership to the application's main loop.
Both callbacks borrow the application, which retains the tracks and the visible cue set throughout dispatch.
The default ``Application::main()`` enters that loop after startup returns.

Follow the End of the First English Cue
=======================================

The backend walks the tracks once to build two entries per cue:

.. erbsland-demo::
    :source: event/apps/CaptionPlayer/src/CaptionBackend.cpp
    :show: c:At equal offsets(+7,+3)
    :sha256: a2fb7a3c580b70e973e8b715883265dd7c9c541f04ee0ca622139e142656346f

.. code-block:: cpp

    for (std::size_t track = 0; track < tracks.size(); ++track) {
        for (std::size_t cue = 0; cue < tracks[track].cues().size(); ++cue) {
            const auto &caption = tracks[track].cues()[cue];
            _timeline.push_back({caption.start, true, track, cue});
            _timeline.push_back({caption.end, false, track, cue});
        }
    }
    // At equal offsets, end old cues first, then retain file and cue order.
    std::ranges::sort(_timeline, {}, [](const CueBoundary &item) {
        return std::tie(item.offset, item.entering, item.track, item.cue);
    });

.. erbsland-demo-end::

At playback start the backend has already recorded one entry for the start of “The tide is turning.” and one for its
end.
It reports the first outstanding deadline to the loop.
When the loop next polls at or after the end offset of 1,200 ms, the backend transfers a copy of the end boundary into a
queued event:

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

The same turn may also queue both commentary changes.
Loop dispatch offers each event to the backend, which checks its ID and payload and calls the display with the stored
boundary.
No parser or terminal code runs in the polling hook.
:doc:`/topics/event/defining_custom_events` follows the payload and :doc:`/topics/event/writing_event_backends`
explains how the loop selects its next wait.

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

Erasing the first English key leaves the second one in the set.
The screen renderer then looks up every remaining cue and prints its lines.
In transcript mode the application prints the single change instead, so several events with the same timestamp remain
individually visible.

Deliver the Last Change and Return
==================================

The timeline cursor may already be at the end while the final cue is still visible.
The backend counts deliveries after the display callback returns.
On the last one it calls the completion callback, which calls ``Application::quit()``.
If posting fails, the poll throws immediately.
A Core runtime error thrown during delivery stops main-loop dispatch and is reported by the application lifecycle.
Neither failure path needs a future caption to bring playback to an end.
