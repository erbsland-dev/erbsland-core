..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Event Loops; Application Designs
    single: Events; Main Loop
    single: Events; Application Parts

***************************************
Running an Application with Event Loops
***************************************

A file monitor needs to prepare its input, read the file periodically, and stop when its work is done.
This page shows how to connect those steps to an application's main event loop.
You will first let the application run the loop for you, then see how to enter it explicitly when you need setup and
cleanup around it.

Both examples are compiled demos: one prints file sizes, the other prints modification times.
If callbacks and timers are unfamiliar, :doc:`events_in_erbsland_core` introduces them using the same file-size monitor.

The Application's Main Event Loop
=================================

For a small monitor, :cpp:class:`Application <erbsland::core::Application>` already provides the surrounding lifecycle.
Its :cpp:func:`run() <erbsland::core::Application::run>` initializes the application, parses the command line, and calls
its default main, which enters the event loop.
The C++ entry point therefore stays small:

.. erbsland-demo::
    :source: event/FileSizeMonitor/main.cpp
    :show: c:This simple main(+0,+8)
    :sha256: 07c954052696e770b20e3b5900b6b5a60286466d3fd9a16e4c1ebc7c96b08335

.. code-block:: cpp

    /// This simple main method is the entry point of the application.
    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableTerminal();
        app.info().setApplicationName("File Size Monitor"_el);
        app.options()->addSet(createOptions());
        return app.run();
    }

.. erbsland-demo-end::

``createOptions()`` registers the file argument, an optional timeout, and a function to handle the parsed values.
That handler prepares the file information and queues the monitor's ``initialize()`` function using
:cpp:func:`invoke() <erbsland::event::Events::invoke>`.
Queuing the function does not call it immediately: it runs when the main loop processes that work.
We will look at the option handler after following the monitor itself.

Timer Setup and Repeated Work
-----------------------------

Once initialization reaches the loop, the monitor prints the current size and starts a repeating timer.
The timer and path information live in ``appData`` so they survive the setup function's return.

.. erbsland-demo::
    :source: event/FileSizeMonitor/main.cpp
    :show: c:This method is called(+5,-2), f:initialize, f:poll
    :exec: event/file_size_monitor {file:text} --timeout 2
    :show-cmd-line:
    :sha256: 3644ffbe79e2b8f2c3feb6cd40c020cab9d297c284a68d396bcde3b4ec1293b3

.. code-block:: cpp

    struct AppData {
        el::EventTimerPtr pollTimer;
        el::PathInfo pathInfo;
    } appData;

    void initialize() {
        el::stdOut()->printLine("Monitoring size of file: "_el, appData.pathInfo.resolvedPath());
        poll();
        appData.pollTimer = el::application().events()->createTimer(poll);
        appData.pollTimer->startFixedDelay(el::Seconds{1});
    }

    void poll() {
        appData.pathInfo.reload();
        if (!appData.pathInfo.exists()) {
            el::stdOut()->printLine("File not found"_el);
        } else {
            el::stdOut()->printLine("Size: "_el, appData.pathInfo.fileSize(), " bytes"_el);
        }
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/file_size_monitor sample-text.txt --timeout 2

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMonitoring size of file: sample-text.txt
        Size: 187 bytes
        Size: 187 bytes␛[0m

.. erbsland-demo-end::

The first ``poll()`` is an ordinary function call made during setup.
Later calls come from the timer, on the same main thread.
:cpp:func:`startFixedDelay() <erbsland::event::EventTimer::startFixedDelay>` leaves a one-second delay after each timer
callback finishes before the next becomes due.
The sample output shows a short run; the readings depend on the file and how long you let the monitor run.

Keeping ``appData.pollTimer`` is essential: releasing its last owning pointer cancels future timer callbacks.
The demo's application-wide state lasts through ``app.run()``.
For a monitor owned by a shorter-lived object, store the timer in that object and ensure its callbacks cannot outlive
the state they access; :doc:`callback_lifetimes` explains those designs.

Configuration Before Dispatch
-----------------------------

The monitor needs a path before its first reading.
An option set's post-parsing callback is a convenient place to turn command-line values into that configuration.
It also schedules the optional quit request, so the demo has a finite run by default.

These two functions complete the setup used by the entry point:

.. erbsland-demo::
    :source: event/FileSizeMonitor/main.cpp
    :show: f:createOptions, f:handleOptions
    :sha256: 49d2222792ebeec1a5084f65ffbc9c80164a634b9d1acb40cccb6e9c56e425ed

.. code-block:: cpp

    auto createOptions() -> el::OptionSetPtr {
        auto optionSet = el::OptionSet::create();
        optionSet->addOption("file"_el).setRequired().setHelp("The path to the file to monitor."_el);
        optionSet->addOption({"-t"_el, "--timeout"_el, "timeout"_el})
            .setType(el::OptionType::Integer)
            .setHelp("The timeout in seconds. Zero for no timeout."_el)
            .setDefaultValue(5);
        optionSet->setPostParsingFn(handleOptions);
        return optionSet;
    }

    void handleOptions(const el::OptionValuesPtr &values) {
        try {
            auto path = el::Path::fromNativeOrThrow(values->getText("file"_el));
            path = path.resolveOrThrow(el::PathResolveMode::Physical);
            appData.pathInfo = path.info(el::PathInfoPart::Type);
        } catch (const el::ParseError &) {
            throw el::ApplicationError{"The path has an invalid format"_el, std::current_exception()};
        } catch (const el::PathError &) {
            throw el::ApplicationError{
                "The file at the given path does not exist or is no regular file"_el, std::current_exception()};
        }
        const auto timeout = el::Seconds{values->getInteger("timeout"_el)};
        if (timeout.isNegative()) {
            throw el::ApplicationError{"The timeout must be a non-negative integer."_el};
        }
        el::application().events()->invoke(initialize);
        if (timeout.isPositive()) {
            el::application().events()->invokeAfter(timeout, []() -> void { el::application().quit(); });
        }
    }

.. erbsland-demo-end::

``setPostParsingFn(handleOptions)`` registers the handler; it does not run it during option registration.
When ``app.run()`` has parsed the arguments, ``handleOptions()`` resolves the path, prepares ``pathInfo``, and rejects a
negative timeout before queuing any work.
Path parsing and resolution failures are translated into application errors with the original cause attached.

The :cpp:func:`invokeAfter() <erbsland::event::Events::invokeAfter>` call queues one delayed action without requiring a
stored timer handle.
A positive timeout ends the run after the requested delay; zero leaves the monitor running until another quit request.
For example, you can run ``file_size_monitor sample-text.txt --timeout 0`` and stop it with Ctrl+C.
The call to ``enableTerminal()`` in the entry point enables the application's terminal integration.

Application Startup and Shutdown
--------------------------------

There are two useful views of this run.
First, the application follows a procedural lifecycle on the main thread.
The event-processing phase occupies the middle of that lifecycle and lasts until the loop exits:

.. mermaid::

    flowchart TD
        Configure["Parse options and prepare the path"]
        Queue["Queue setup and the optional timeout"]
        Run[["Run the main event loop"]]
        Finish["Finish shutdown and return the exit code"]
        Configure --> Queue --> Run --> Finish

Inside the event-processing phase, the loop dispatches one callback at a time.
The queued setup runs once, timer callbacks provide later readings, and the timeout callback requests shutdown.
Each callback returns control to the loop:

.. mermaid::

    flowchart TD
        Dispatch["Dispatch ready work"]
        Setup["initialize()<br/>Start monitoring"]
        Poll["poll()<br/>Read file size"]
        Quit["quit()<br/>Request shutdown"]
        Exit["Return; loop exits"]
        Dispatch -->|Queued setup| Setup
        Setup -->|Return| Dispatch
        Dispatch -->|Polling timer| Poll
        Poll -->|Return| Dispatch
        Dispatch -->|Timeout| Quit
        Quit --> Exit

The arrows back to dispatch are returns from callbacks, not calls that your code must make.
If nothing is ready, the loop waits.
These are two views of the same main thread; the timer does not introduce another thread.

Calling :cpp:func:`quit() <erbsland::core::Application::quit>` requests shutdown; it does not terminate the process
inside the callback.
The callback returns, the loop exits, and ``app.run()`` completes the application lifecycle.
There is no separate thread for this monitor: configuration, setup, and readings all run on the main thread.
The loop makes it possible to wait between readings while remaining available for other events.

Errors also have a path out of the monitor.
A setup error or an exception from a timer callback reaches the application's error handling.
Resources that need unconditional release should have an owning object or a non-throwing cleanup hook, as in the next
example.
The :ref:`event-loop-options` reference describes error policies for loops you configure yourself.

Initialization and Cleanup Hooks
================================

Sometimes the event-processing phase is one step in a larger sequence.
You may want to open resources first and continue with final work after the loop stops.
Deriving from ``Application`` and overriding its ``main()`` gives you that sequence.
Your override explicitly calls :cpp:func:`runEventLoop() <erbsland::core::Application::runEventLoop>` where event
processing should begin.

The file-time monitor uses this arrangement.
It keeps its path information and timer as members, validates the input, and starts the timer before entering the loop.
Timers can be prepared before dispatch starts; other event sources may require setup from inside their owning loop, as
shown in :doc:`using_event_apis`.

.. erbsland-demo::
    :source: event/FileTimeMonitor/FileTimeMonitor.hpp
    :hide: f:initialize, f:registerCommandLineOptions
    :exec: event/file_time_monitor {file:text} --timeout 2
    :show-cmd-line:
    :sha256: 54f3137673d85a656a4af766d415ff7ba21a87a601d08e69e6e4a5aca3da80b9

.. code-block:: cpp

    /// Initialize resources, enter the main event loop, and release resources when it returns.
    /// `PathInfo` reloads filesystem timestamps for each timer callback.
    /// @notest{Verified by running the file_time_monitor demo.}
    class FileTimeMonitorApp final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application

        [[nodiscard]] auto main() -> el::ExitCode override {
            _path = el::Path::fromNativeOrThrow(optionValues()->getText("file"_el))
                        .resolveOrThrow(el::PathResolveMode::Physical);
            _pathInfo = _path.info(el::PathInfoPart::Type);
            if (!_pathInfo.isRegularFile()) {
                throw el::ApplicationError{"The path must name a regular file."_el};
            }
            const auto timeout = el::Seconds{optionValues()->getInteger("timeout"_el)};
            if (timeout.isNegative()) {
                throw el::ApplicationError{"The timeout must be non-negative."_el};
            }
            if (timeout.isPositive()) {
                events()->invokeAfter(timeout, []() -> void { el::application().quit(); });
            }
            _pollTimer = events()->createTimer([this]() -> void { poll(); });
            _pollTimer->startFixedDelay(el::Seconds{1});
            el::stdOut()->printLine("Monitoring file: "_el, _path);
            // Enter the event loop after setup; execution resumes here when it exits.
            const auto exitCode = runEventLoop();
            _pollTimer->stop();
            el::stdOut()->printLine("Monitoring stopped."_el);
            return exitCode;
        }
        void cleanup() noexcept override { _pollTimer.reset(); }

    private:
        void poll() {
            _pathInfo.reload(el::PathInfoPart::Times);
            el::stdOut()->printLine("Last modification: "_el, _pathInfo.lastModified().toString());
        }

    private:
        el::EventTimerPtr _pollTimer; ///< Retains the repeating timer.
        el::Path _path;               ///< File being monitored.
        el::PathInfo _pathInfo;       ///< Reloadable filesystem information.
    };

.. erbsland-demo-run::

    .. code-block:: console

        $ event/file_time_monitor sample-text.txt --timeout 2

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMonitoring file: sample-text.txt
        Last modification: 2026-11-06T10:32:10.986459848Z
        Monitoring stopped.␛[0m

.. erbsland-demo-end::

The callback captures ``this`` to reach the monitor's members.
The application object remains alive throughout ``app.run()``, and its timer is released during cleanup, so those
members remain available while callbacks execute.
The entry point constructs ``FileTimeMonitorApp`` in place of ``Application`` and calls ``app.run()`` in the same way.
The timestamp in the captured output is from an example run.

``runEventLoop()`` stays inside the overridden ``main()`` until the loop exits.
On the normal path, execution then stops the timer, prints the final message, and returns the loop's exit code.
Overriding ``main()`` replaces the default behavior, so forgetting the explicit loop call would leave the scheduled
callbacks unprocessed.

The :cpp:func:`cleanup() <erbsland::core::Application::cleanup>` hook handles release even if setup or a callback throws
and bypasses the statements after ``runEventLoop()``.
Keep that hook non-throwing and let resource-owning members handle ordinary destruction.
This is the main reason to distinguish work that follows a successful event-processing phase from cleanup needed on
every exit path.

Worker Event Threads
====================

Both monitor designs use one loop on the main thread.
That is enough while each callback finishes promptly.
If a callback instead hashes a large file, the monitor cannot process other callbacks until hashing returns.
An additional event thread gives that work its own loop and lets the main loop remain responsive.

A batch application can use the same idea while keeping a procedural main: send files to workers, wait for their
results, then write the report.
:doc:`processing_work_on_event_threads` develops this design with the file-hasher demo.
It explains why a main function that waits without running its loop needs another place to receive completion callbacks.

.. _event-application-parts:

Application Parts and Their Event Loops
=======================================

A long-lived service may need its own event sources, startup, and asynchronous shutdown.
An :cpp:class:`ApplicationPart <erbsland::core::ApplicationPart>` gives those responsibilities a dedicated event thread
whose lifecycle the application manages.

:doc:`using_event_loops_in_application_parts` follows a file server from configuration through request callbacks to
graceful shutdown.
It explains which work belongs on the part's thread and how that loop stays active while the server finishes closing.
