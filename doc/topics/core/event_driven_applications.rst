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
    :source-sha256: 05a4f249e0eeb88d7a28d1a4fe49b4b6fca7ab76fcdfe26990b5fddc02042c13

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
            events()->invoke([this]() -> void { runMaintenance(); });
        }

    private:
        void runMaintenance() {
            el::io::printLine("maintenance job started"_el);
            events()->invoke([this]() -> void {
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

.. rubric:: ``$ core/core_event_application``

.. erbsland-ansi::
    :escape-char: ␛

    maintenance job started
    maintenance job completed

.. rubric:: ``$ core/core_event_application --help``

.. erbsland-ansi::
    :escape-char: ␛

    Usage:
    core_event_application [options]
    Options:
    -h, --help[=<boolean>]    Display this help.
        --version[=<boolean>] Display version information.

.. erbsland-demo-end::

Starting Work at the Right Time
===============================

``initialize()`` is useful for metadata, terminal setup, shared services, and work that must exist before parsing.
``registerCommandLineOptions()`` describes accepted arguments.
When asynchronous work depends on parsed values, start it after the base parser has succeeded, for example from an
overridden ``parseCommandLine()`` that first calls ``Application::parseCommandLine()`` and checks ``optionValues()``.

Queued initialization events are harmless when the user requests help or version output because the application never
enters the event loop in that case.
Avoid starting unmanaged background work during initialization, because it would need separate cancellation.

Finishing and Reporting Failures
================================

The first call to ``quit()`` chooses the process exit code.
It also asks registered application parts and managed event threads to stop before the main event system exits.

An Erbsland exception thrown by a callback on the automatically managed main event loop crosses the same reporting
boundary as an exception from ``initialize()`` or ``main()``.
Use explicit result handling when a failure is expected and recoverable; throw ``ApplicationError`` when a callback must
terminate the whole process with a diagnostic and exit code.

Overriding ``main()`` disables the automatic event-loop entry.
An application that needs custom synchronous preparation before dispatch can finish its override with
``runEventLoop()``.
