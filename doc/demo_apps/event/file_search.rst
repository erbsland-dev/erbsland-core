..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Demo Applications; FileSearch

**************************************
FileSearch: Interactive Path Selection
**************************************

``FileSearch`` helps you find and select a path without leaving the terminal.
Type a glob, browse matches as they arrive, and press Enter to print the selected absolute path after the application
restores your terminal.
Directory traversal runs on a managed event thread, keeping the main loop available for editing, navigation, and screen
updates.

The application demonstrates a practical request-and-reply protocol: each edit replaces the current request, worker
batches own their data, and replies for older requests are rejected.
It also shows why cancellation and reply validation solve different problems.
:doc:`/topics/event/communicating_between_loops` introduces the general pattern.

Finding and Selecting a Path
============================

Build the ``file_search`` target, then run it in an interactive terminal:

.. code-block:: console

    $ cmake --build cmake-build-debug --target file_search
    $ cmake-build-debug/demo-apps/event/file_search .

The optional directory defaults to the current directory.
The screen has framed areas for the absolute root, the match list and its status, and the editable pattern.
An empty query starts no search.
Try ``**/*.hpp`` to find headers at any depth, or ``src/**/*.cpp`` to restrict the search to a subtree.
Patterns use
:doc:`/reference/path/path_glob_patterns` and match complete paths relative to the root: ``*.cpp`` matches only the root
directory.
Matching is case-sensitive and includes hidden entries.
Files, directories, and symbolic links can appear; symlinked directories are not traversed.

The first match is selected automatically.
Up/Down move one row and PageUp/PageDown move by the visible page size.
New matches append without moving your selection.
Long paths are cropped on the left, retaining their filenames; the selection always retains the full path.
Left/Right and Home/End move the input cursor, and Backspace/Delete edit the pattern.
Combining marks stay with their base character during editing.
The cursor blinks while results arrive.

Enter exits and prints the selected path, or nothing if there is no selection.
Escape exits without printing a path.
Both restore the previous terminal screen.
A small terminal displays a resize message while still accepting exit keys.
The program requires an interactive terminal and does not turn redirected input into a different search mode.

Keeping Main Responsive
=======================

``FileSearchApp`` keeps the editor, result list, selection, and screen buffer on main.
It retains the managed worker through application shutdown.
An atomic generation counter carries query replacement, while the worker's stop token carries permanent thread shutdown:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.hpp
    :sha256: f233f01dcfc14841686567818a05051c9969ec3fdf78ca737a2980da8faecf74

.. code-block:: cpp

    /// Search a directory while editing a glob and selecting a matching path on main.
    class FileSearchApp final : public el::Application {
        static constexpr auto cDirectoryOption = el::StringLiteral{"directory"};
        using Generation = std::atomic<std::uint64_t>;
        using GenerationPtr = std::shared_ptr<Generation>;

    public:
        using Application::Application;

    protected: // implement Application
        void initialize() override;
        void registerCommandLineOptions(const el::OptionsPtr &options) override;
        void handleCommandLineOptions(const el::OptionValuesPtr &values) override;
        void cleanup() noexcept override;

    private:
        void begin();
        void tick();
        void queryChanged();
        void submitSearch();
        void requestExit(bool select);

    private:
        el::Path _root;                                              ///< Absolute root, fixed before dispatch.
        QueryEditor _query;                                          ///< Main-loop input state.
        SearchResults _results;                                      ///< Main-loop result state.
        SearchView _view;                                            ///< Main-loop presentation.
        el::ManagedEventThreadPtr _worker;                           ///< Retained until Application joins it.
        el::EventTimerPtr _refresh;                                  ///< Polls keys and renders at approximately 30 Hz.
        el::EventTimerPtr _debounce;                                 ///< Starts only the latest edited query.
        el::ElapsedTimer _blink;                                     ///< Cursor phase, reset by keyboard input.
        GenerationPtr _generation = std::make_shared<Generation>(0); ///< The worker reads cancellation, not UI state.
        el::block::Size _lastSize;                                   ///< Screen size of the last rendered frame.
        bool _dirty{true};                                           ///< Input or results changed since rendering.
        bool _cursorVisible{false};                                  ///< Cursor phase of the last rendered frame.
        bool _exiting{false};                                        ///< Suppresses late rendering and result deliveries.
    };

.. erbsland-demo-end::

Startup selects ``FailApplication`` for the worker, puts terminal input into key mode, and creates two main-loop timers.
The refresh timer polls a bounded number of keys without blocking, then renders at approximately 30 Hz.
The debounce timer waits for 150 milliseconds of unchanged input before starting a search:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: f:FileSearchApp::begin, f:FileSearchApp::queryChanged
    :sha256: 6ede999aa40305427daf1fbaf2fc57bbfa396549053e62cc8646c10fb049b461

.. code-block:: cpp

    void FileSearchApp::begin() {
        _worker = createEventThread(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication));
        _worker->start();
        terminal()->input().setMode(el::cterm::Input::Mode::Key);
        _results.setStatus("Type a glob: *.cpp at root, **/*.cpp recursively."_el);
        _debounce = loop()->createTimer([this]() -> void { submitSearch(); });
        _refresh = loop()->createTimer([this]() -> void { tick(); });
        _refresh->startFixedDelay(el::Milliseconds{33});
        tick();
    }

    void FileSearchApp::queryChanged() {
        _results.reset();
        _generation->store(_results.generation());
        _debounce->stop();
        if (_query.text().isEmpty()) {
            _results.setStatus("Type a glob: *.cpp at root, **/*.cpp recursively."_el);
            return;
        }
        _results.setStatus("Waiting for input…"_el);
        _debounce->startOnce(el::Milliseconds{150});
    }

.. erbsland-demo-end::

Every actual edit invalidates the previous generation immediately, even while the replacement is waiting for its
debounce timer.
Empty input cancels without replacement.
A malformed pattern displays an inline error and leaves the editor active.
``QueryEditor`` uses Core string operations and ``cterm`` key events; the standalone ``ReadLine`` editor owns its output
area and exposes committed text, so it does not fit this screen's live editing protocol.

Returning Owned Batches
=======================

The submission callback compiles a pattern and binds the reply on main before sending work to the worker:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: f:FileSearchApp::submitSearch
    :sha256: e87d636e922337efc6db4a379eb47f33ab2d38aa3f083cf13615758dbb591924

.. code-block:: cpp

    void FileSearchApp::submitSearch() {
        if (_exiting) {
            return;
        }
        _dirty = true;
        // Compile the current input before starting another search.
        auto pattern = el::PathGlobPatternPtr{};
        try {
            pattern = el::PathGlobPattern::compile(_query.text());
        } catch (const el::ParseError &error) {
            // An unfinished glob is ordinary editable input, not an application failure.
            _results.setStatus(el::join("Invalid glob: "_el, error.reason()));
            return;
        }
        _results.setStatus("Searching…"_el);
        const auto generation = _results.generation();
        // Bind on main before crossing to the worker. The reply retains this request's identity.
        auto reply = el::bindEvent([this, generation](SearchUpdate update) -> void {
            if (!_exiting) {
                _results.receive(generation, std::move(update));
                _dirty = true;
            }
        });
        _worker->loop()->invoke(
            [root = _root,
                pattern = std::move(pattern),
                generation,
                token = _generation,
                stopToken = _worker->stopToken(),
                reply = std::move(reply)]() -> void {
                const auto cancelled = [token, generation, stopToken]() -> bool {
                    return token->load() != generation || stopToken.isStopRequested();
                };
                SearchScanner{root, pattern}.scan(cancelled, reply);
            });
    }

.. erbsland-demo-end::

The compiled glob is immutable.
The worker receives its own root path and reply binding, and never reads the editor or writes the displayed results.
The reply retains the query identity and transfers each ``SearchUpdate`` back to main.
Its borrowed application pointer remains valid because the application outlives managed-worker shutdown.
The worker's copied ``StopToken`` observes shutdown independently of the query generation and does not retain the thread
or borrow its loop.

``SearchScanner`` uses Core's deterministic path walker, matching relative paths and pruning directories that cannot
contain a match.
Cancellation is checked before matching every visited entry, not only when a match is found.
This matters when a search has no results.
The scanner emits at most 128 paths per batch and flushes partial batches when at least 50 milliseconds have elapsed at
a visit, or when the search finishes:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/SearchScanner.cpp
    :show: f:SearchScanner::scan, f:SearchScanner::visit, f:SearchScanner::deliverBatch
    :sha256: f66b21c6c76e14588d2af1670b8540c79ae287f85ea15af8782c8cb38ddf9846

.. code-block:: cpp

    void SearchScanner::scan(IsCancelled isCancelled, OnUpdate onUpdate) {
        _isCancelled = std::move(isCancelled);
        _onUpdate = std::move(onUpdate);
        _batch = {};
        _lastDelivery.restart();
        if (_isCancelled()) {
            return;
        }
        // Visit nonmatching entries too: cancellation must work even when there are no matches.
        const auto options = el::PathWalkOptions{}
                                 .setTypes(el::PathType::All)
                                 .setSymlinkMode(el::SymlinkMode::Use)
                                 .setInfoParts(el::PathInfoPart::Type)
                                 .setIgnoreErrors(true);
        const auto outcome = _root.walker().walkOrThrow(
            [this](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus { return visit(path, info); },
            options);
        if (!_isCancelled()) {
            _batch.finished = true;
            _batch.incomplete = outcome == el::PathWalkResult::Failure;
            deliverBatch();
        }
    }

    auto SearchScanner::visit(const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
        if (_isCancelled()) {
            return el::PathWalkStatus::Stop;
        }
        if (path == _root) {
            return el::PathWalkStatus::Continue;
        }
        const auto relative = path.toRelativeOrThrow(_root);
        const auto type = info.type();
        if ((type == el::PathType::RegularFile || type == el::PathType::Directory || type == el::PathType::Symlink) &&
            _pattern->fullMatch(relative)) {
            _batch.paths.append(path);
        }
        if (_batch.paths.count() >= el::ItemCount{128} ||
            (!_batch.paths.isEmpty() && _lastDelivery.elapsed() >= el::Milliseconds{50})) {
            deliverBatch();
        }
        if (type == el::PathType::Directory && !_pattern->mayMatchDescendant(relative)) {
            return el::PathWalkStatus::Skip;
        }
        return el::PathWalkStatus::Continue;
    }

    void SearchScanner::deliverBatch() {
        _onUpdate(std::move(_batch));
        _batch = {};
        _lastDelivery.restart();
    }

.. erbsland-demo-end::

Filesystem errors are recorded by the walker's outcome while other accessible entries remain searchable.
Completion carries an ``incomplete`` flag when errors occurred; the UI visibly distinguishes partial results from a
complete search.
There is no arbitrary result limit: the current query's matches remain available for scrolling.

Rejecting Obsolete Results
==========================

A cancelled scanner may already have posted a batch.
Cancellation saves work, but cannot retract that delivery.
The receiver checks the generation before accepting paths or completion status:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/SearchResults.cpp
    :show: f:SearchResults::receive
    :sha256: 78e228e94f9f4d809c3de0b2e3b74bba0010df1c70baf1fb88cffeb7f76f97c3

.. code-block:: cpp

    void SearchResults::receive(const std::uint64_t generation, SearchUpdate update) {
        if (generation != _generation) {
            return;
        }
        _paths.append(update.paths);
        if (update.finished) {
            if (update.incomplete) {
                _status = "Incomplete: some paths could not be read."_el;
            } else if (_paths.isEmpty()) {
                _status = "No matches."_el;
            } else {
                _status = "Search complete."_el;
            }
        }
    }

.. erbsland-demo-end::

Try changing ``**/*.hpp`` to ``**/*.cpp`` while a large directory is being scanned.
The old selection disappears as soon as input changes.
A late header batch cannot enter the new list, and an old completion cannot replace its progress message.
This check would remain necessary with multiple workers or a different cancellation mechanism.

Stopping Without Waiting for a Reply
====================================

Escape and Enter invalidate the query, stop the timers, and restore the terminal before asking the application to quit.
Enter prints the retained selected path only after leaving the alternate screen:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: f:FileSearchApp::requestExit, f:FileSearchApp::cleanup
    :sha256: d315b21d7fedb95c04f075b3db962c28dc59a8f01e60b79e23c878a0b0ee570a

.. code-block:: cpp

    void FileSearchApp::requestExit(const bool select) {
        _exiting = true;
        _generation->fetch_add(1);
        _refresh->stop();
        _debounce->stop();
        // Restore the user's terminal before waiting for an outstanding filesystem call.
        terminal()->restoreScreen();
        if (select) {
            if (const auto path = _results.selectedPath()) {
                el::io::printLine(path->toNativeString());
                el::stream::stdOut()->flush();
            }
        }
        quit();
    }

    void FileSearchApp::cleanup() noexcept {
        _exiting = true;
        _generation->fetch_add(1);
        if (_refresh) {
            _refresh->stop();
        }
        if (_debounce) {
            _debounce->stop();
        }
        terminal()->restoreScreen();
        // Application has joined the worker before cleanup, including on failure.
        _worker.reset();
    }

.. erbsland-demo-end::

The scanner observes both generation changes and the worker's stop token.
Consequently application shutdown can cancel it even after main stops receiving replies.
An application failure may begin joining workers before ``cleanup()`` runs, so changing the generation only during
cleanup would be too late.
No final reply is required to release a join.
Queued searches check cancellation before doing useful work, and the application joins its managed worker before
releasing the handle.
An operating-system filesystem call already in progress is not interruptible through this API, so a stalled filesystem
can delay process exit even though the terminal has been restored.

Uncaught worker ``RuntimeError`` exceptions use ``FailApplication`` and the standard diagnostic boundary.
Invalid glob syntax is handled locally, and partial filesystem results have an explicit outcome.
Programming errors remain fatal.
:doc:`/topics/event/handling_event_errors` explains these boundaries and the distinction between loop termination and
waiting in ``join()``.
