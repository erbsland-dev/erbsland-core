..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************************************
FileSizeMonitor: A Direct Application
*************************************

``FileSizeMonitor`` shows how a small event-driven application can use
:cpp:class:`Application <erbsland::core::Application>` directly.
It reads a file path from command-line configuration, starts repeated work on the main loop, and retains the timer that
keeps that work active.
A timeout supplies a finite completion path.
The polling itself is deliberately small so the relationship between configuration, startup, and dispatch remains
visible.
For the general startup choices, see :doc:`/topics/event/using_event_loops`.

Configuration Before Dispatch
=============================

The entry point constructs an application, adds the option set, and calls ``run()``.
It leaves the default main intact, so the framework will enter main-loop dispatch after successful configuration.
``createOptions()`` connects ``handleOptions()`` with ``setPostParsingFn()``:

.. erbsland-demo::
    :source: event/apps/FileSizeMonitor/main.cpp
    :show: f:createOptions
    :sha256: fd04e5cd03c33dee16c2633b43c25f73dc1a5014205fb0bd2c411e9c55beff3e

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

.. erbsland-demo-end::

The post-parsing callback uses the values it receives, resolves the input path, and retains its
:cpp:class:`PathInfo <erbsland::path::PathInfo>` in ``appData``.
It rejects negative timeouts with :cpp:class:`ApplicationError <erbsland::core::ApplicationError>` and adds context to
path failures.
Only after configuration succeeds does it queue ``initialize()``.
It also queues application quit after a positive timeout; zero means that no timed exit is installed.

.. erbsland-demo::
    :source: event/apps/FileSizeMonitor/main.cpp
    :show: f:handleOptions
    :sha256: 294f99f2dba60aa47aded279a51ae1be3e71adddb67974fd86092303a677defe

.. code-block:: cpp

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
        el::application().loop()->invoke(initialize);
        if (timeout.isPositive()) {
            el::application().loop()->invokeAfter(timeout, []() -> void { el::application().quit(); });
        }
    }

.. erbsland-demo-end::

Although configuration runs on the main thread, the queued ``initialize()`` runs later inside its event loop.
The default main supplies that transition; calling ``loop()`` during configuration does not itself start dispatch.

Retaining Repeated Work
=======================

``appData`` has static lifetime in this small demo.
Its path information survives the configuration callback, and its ``pollTimer`` retains the repeated activity after
``initialize()`` returns.
Both initialization and subsequent polls execute on the main loop, so they access this state sequentially.

.. erbsland-demo::
    :source: event/apps/FileSizeMonitor/main.cpp
    :show: c:This method is called(+5,-2), f:initialize, f:poll
    :sha256: 0263113aa63ded63048dbec529974593041a586007b1acad9178497311d1f4cb

.. code-block:: cpp

    struct AppData {
        el::EventTimerPtr pollTimer;
        el::PathInfo pathInfo;
    } appData;

    void initialize() {
        el::stdOut()->printLine("Monitoring size of file: "_el, appData.pathInfo.resolvedPath());
        poll();
        appData.pollTimer = el::application().loop()->createTimer(poll);
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

.. erbsland-demo-end::

Initialization performs one immediate poll, then starts a fixed-delay timer.
Each timer callback reloads the file information and prints the current size or reports that the file was not found.
A timer stored only in a local variable would lose its last owner on return and would not sustain polling.
A larger application would normally put this state in an owned component instead of a global object.
:doc:`/topics/event/scheduling_work` explains the timer modes and their timing guarantees.

Completion and a Bounded Run
============================

The delayed quit request ends main-loop dispatch without interrupting a poll already running.
The application then completes its shutdown lifecycle.
This demo's global timer pointer remains until program teardown; it does not keep dispatch running after quit.
For explicit member cleanup, compare :doc:`file_time_monitor`.

This example monitors a disposable text file for two seconds.
The first size is printed during initialization; the number of later polls depends on dispatch timing near shutdown.

.. erbsland-demo::
    :source: event/apps/FileSizeMonitor/main.cpp
    :show: c:This simple main method(+0,+7)
    :exec: event/file_size_monitor {file:text} --timeout 2
    :show-cmd-line:
    :sha256: d6ae168dc252a8883de88f60f4a124f2d82eadbde43389aa7cd2f4f14dc8cdbc

.. code-block:: cpp

    /// This simple main method is the entry point of the application.
    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableTerminal();
        app.info().setApplicationName("File Size Monitor"_el);
        app.options()->addSet(createOptions());
        return app.run();
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
