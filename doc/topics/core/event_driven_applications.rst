..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application; Event Driven
    single: Application; Event Loop
    single: Application; quit

**********************************
Building Event-Driven Applications
**********************************

An event-driven application begins work and then reacts to completions, timers, input, or service notifications.
The default :cpp:func:`Application::main() <erbsland::core::Application::main>` supplies the main event loop, so the
derived class only needs to arrange the first event and call ``quit()`` when the process should finish.

Entering the Main Event Loop
============================

The following application schedules its first task during initialization.
After command-line parsing succeeds, the unchanged base ``main()`` enters the event loop and dispatches the queued work.
The second callback calls :cpp:func:`quit() <erbsland::core::Application::quit>`, which records the exit code and ends
managed event processing in an orderly way.

.. erbsland-demo::
    :source: core/EventApplication/main.cpp
    :exec: core/core_event_application
    :exec-2: core/core_event_application --help
    :sha256: 59499f11b40ccd38666eed9774598ab36b22c71fc6eda71797e3f4210406fa50

.. code-block:: cpp

    /// Derive from `Application` when the program is driven by asynchronous work on the main event loop.
    ///
    /// The default `Application::main()` enters the event loop.
    /// Calling `quit()` from an event callback finishes the loop and returns the requested exit code.
    class MaintenanceApplication final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application
        void initialize() override {
            info().setApplicationName("Maintenance Queue"_el);
            info().setApplicationVersion(el::Version{1, 0, 0});
            loop()->invoke([this]() -> void { runMaintenance(); });
        }

    private:
        void runMaintenance() {
            el::io::printLine("maintenance job started"_el);
            loop()->invoke([this]() -> void {
                el::io::printLine("maintenance job completed"_el);
                quit();
            });
        }
    };

    /// Create the application and leave event dispatch to its default main implementation.
    auto main(const int argc, char *argv[]) -> int {
        auto app = MaintenanceApplication{argc, argv};
        return app.run();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ core/core_event_application

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mmaintenance job started
        maintenance job completed␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ core/core_event_application --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mUsage:
        core_event_application [options]
        Options:
        -h, --help[=<keyword>]  Display this help.
            --version           Display version information.␛[0m

.. erbsland-demo-end::

Starting Asynchronous Work after Option Parsing
===============================================

``initialize()`` is useful for metadata, terminal setup, shared services, and work that must exist before parsing.
``registerCommandLineOptions()`` describes accepted arguments.
When asynchronous work depends on parsed values, override ``handleCommandLineOptions(const OptionValuesPtr &values)``.
The application calls this hook after successful parsing, before forwarding options to application parts and entering
``main()``.
The passed values are non-null; help, version output, module overview, and parsing errors skip this hook.
Validate inputs and schedule the first event there so the base ``main()`` still supplies event dispatch.

For example, a directory packer can resolve its source and queue a callback that starts scanning:

.. code-block:: cpp

    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        const auto source = el::Path::fromNativeOrThrow(values->getText("source"_el))
                                .resolveOrThrow(el::PathResolveMode::Physical);
        loop()->invoke([this, source]() -> void { startScanning(source); });
    }

Here ``source`` is a required text option registered by the application, and ``startScanning()`` begins its workflow on
the main loop.
A workflow using raw worker threads must retain their owners until those threads have been cancelled and joined.
Only override ``parseCommandLine()`` when the parsing behavior itself needs to change.

Queued initialization events are harmless when the user requests help or version output because the application never
enters the event loop in that case.
Avoid starting unmanaged background work during initialization, because it would need separate cancellation.

Finishing and Reporting Failures
================================

The first call to ``quit()`` chooses the process exit code.
It also asks registered application parts and managed event threads to stop before the main event system exits.

An Erbsland Core exception thrown by a callback on the automatically managed main event loop crosses the same reporting
boundary as an exception from ``initialize()`` or ``main()``.
Use explicit result handling when a failure is expected and recoverable; throw ``ApplicationError`` when a callback must
terminate the whole process with a diagnostic and exit code.

Overriding ``main()`` disables the automatic event-loop entry.
Preparation that depends on options belongs in ``handleCommandLineOptions()`` and keeps that entry intact.
A custom main that also needs work after dispatch can call ``runMainEventLoop()`` explicitly.
