..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**************************************
FileTimeMonitor: Setup Around Dispatch
**************************************

``FileTimeMonitor`` demonstrates a derived application that performs procedural setup, explicitly runs its main event
loop, and resumes procedural code after dispatch ends.
It retains a polling timer as a member and releases it in ``cleanup()``.
This design is useful for understanding the boundary between application lifecycle code and dispatched callbacks;
:doc:`/topics/event/using_event_loops` also shows designs that retain the default main.

The Application Owns the Monitor
================================

``FileTimeMonitorApp`` stores the selected path, reloadable file information, and timer in members.
``initialize()`` sets up terminal support and application metadata.
``registerCommandLineOptions()`` declares the required file and an optional timeout; neither hook starts polling.
The inherited ``run()`` still handles command-line processing and calls the overridden main only after it succeeds.

.. erbsland-demo::
    :source: event/apps/FileTimeMonitor/FileTimeMonitor.hpp
    :show: f:initialize, f:registerCommandLineOptions, c:Retains the repeating timer(+0,+2)
    :sha256: 5bdf591e27d23c0b752b0e82c856cc663097b4eeede768d27b061eb80d0e3f87

.. code-block:: cpp

    void initialize() override {
        enableTerminal();
        info().setApplicationName("File Time Monitor"_el);
    }

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption("file"_el).setRequired().setHelp("The path to the file to monitor"_el);
        options->addOption({"-t"_el, "--timeout"_el, "timeout"_el})
            .setType(el::OptionType::Integer)
            .setHelp("The timeout in seconds. Zero for no timeout."_el)
            .setDefaultValue(5);
    }

    el::EventTimerPtr _pollTimer; ///< Retains the repeating timer.
    el::Path _path;               ///< File being monitored.
    el::PathInfo _pathInfo;       ///< Reloadable filesystem information.

.. erbsland-demo-end::

A Procedural Main Around the Loop
=================================

``main()`` resolves and validates the file, reads the timeout, and prepares the timer on the main thread.
The timer scheduler supports this setup before dispatch; it does not establish a current-loop context for arbitrary
source factories.
Factories that require that context must be called from a queued callback, as explained in
:doc:`/topics/event/events_in_erbsland_core`.

The explicit :cpp:func:`runMainEventLoop() <erbsland::core::Application::runMainEventLoop>` call is what lets the
scheduled callbacks execute.
The native thread remains the main thread throughout: procedural setup, loop dispatch, and the code after dispatch are
successive phases on the same thread.

.. erbsland-demo::
    :source: event/apps/FileTimeMonitor/FileTimeMonitor.hpp
    :show: c:implement Application(-12,+34)
    :sha256: 1e3efaa00656869a9e58bdac311cfdb388a80aac75dba67b0e908a78400bd82a

.. code-block:: cpp

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
            loop()->invokeAfter(timeout, []() -> void { el::application().quit(); });
        }
        _pollTimer = loop()->createTimer([this]() -> void { poll(); });
        _pollTimer->startFixedDelay(el::Seconds{1});
        el::stdOut()->printLine("Monitoring file: "_el, _path);
        // Enter the event loop after setup; execution resumes here when it exits.
        const auto exitCode = runMainEventLoop();
        _pollTimer->stop();
        el::stdOut()->printLine("Monitoring stopped."_el);
        return exitCode;
    }

.. erbsland-demo-end::

A positive timeout queues ``Application::quit()``; zero leaves the monitor running.
Each poll reloads timestamps and prints the current modification time.
No other callback on the main loop can run until that poll returns.

.. erbsland-demo::
    :source: event/apps/FileTimeMonitor/FileTimeMonitor.hpp
    :show: f:poll, f:cleanup
    :exec: event/file_time_monitor {file:text} --timeout 2
    :show-cmd-line:
    :sha256: 0d4f636acfc26c1fec712032f8f03454b623c504e480784f4cc896dfab0ceee1

.. code-block:: cpp

    void poll() {
        _pathInfo.reload(el::PathInfoPart::Times);
        el::stdOut()->printLine("Last modification: "_el, _pathInfo.lastModified().toString());
    }

    void cleanup() noexcept override { _pollTimer.reset(); }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/file_time_monitor sample-text.txt --timeout 2

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMonitoring file: sample-text.txt
        Last modification: 2026-11-11T16:53:19.466089775Z
        Monitoring stopped.␛[0m

.. erbsland-demo-end::

Stopping and Cleanup
====================

When dispatch returns normally, ``main()`` stops the timer, prints its final message, and returns the loop's exit code.
The framework then calls ``cleanup()``, which resets the retained timer.
That cleanup also handles failures that bypass the ordinary code after ``runMainEventLoop()``.
Resetting a possibly empty pointer tolerates initialization that ended partway through.

An invalid file or timeout produces an application diagnostic before monitoring begins.
A callback failure is handled through the surrounding application lifecycle.
Cleanup does not depend on another callback, because it runs outside main-loop dispatch.
:doc:`/topics/event/shutting_down_event_work` explains this boundary for operations with more involved shutdown needs.
