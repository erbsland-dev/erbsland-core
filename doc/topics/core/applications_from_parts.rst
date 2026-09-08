..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application; Parts
    single: ApplicationPart; Application Integration
    single: Application; registerPart

**********************************
Building an Application from Parts
**********************************

Application parts divide a process into services with explicit dependencies and independent event loops.
:cpp:class:`Application <erbsland::core::Application>` can own their manager, connect their command-line hooks, and
coordinate startup and shutdown with its main event loop.

Registering the Service Graph
=============================

Register concrete part classes before ``run()``.
The application prepares the complete dependency graph after its own initialization and before option registration.
Preparation validates every registration and creates the part objects, but it does not start their event threads yet.

The default application main starts all automatic parts and enters the main event loop.
When ``quit()`` is called, the application first stops the part graph in reverse dependency order and only then closes
the remaining managed event system.

.. erbsland-demo::
    :source: core/ApplicationParts/ApplicationMain.cpp
    :exec: core/core_part_application
    :source-sha256: f64f51596a88f04da7c579ba40013403e0123bede310a397aba960fecd6074a0

.. code-block:: cpp

    /// Register application parts before `run()` and let the default application main manage their lifecycle.
    ///
    /// The application prepares the dependency graph, starts automatic parts, enters the main event loop, and coordinates
    /// reverse dependency shutdown before returning from `run()`.
    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.info().setApplicationName("Catalog Service"_el);
        app.registerPart<CatalogStoragePart>();
        app.registerPart<CatalogServerPart>();
        [[maybe_unused]] auto stateSubscription =
            app.partManager()->events().addStateChanged([&app](const el::ApplicationPartManagerState state) -> void {
            if (state == el::ApplicationPartManagerState::Running) {
                el::io::printLine("all application parts are running"_el);
                app.quit();
            }
            });
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛

    catalog storage initialized
    catalog server initialized; catalog storage is ready
    all application parts are running
    catalog server cleaned up
    catalog storage cleaned up

.. erbsland-demo-end::

Keeping Application and Service Responsibilities Separate
=========================================================

The application remains responsible for process-wide concerns such as metadata, root options, terminal setup, and the
final exit code.
Parts own service-specific state, event callbacks, and dependencies.
This separation lets a service be reused with a detached manager while the application stays a small composition root.

Each part can contribute options through ``registerCommandLineOptions()`` and receive successfully parsed values through
``parseCommandLine()``.
Help and parsing errors do not start the graph.

Accessing Prepared Services
===========================

After preparation, :cpp:func:`Application::part() <erbsland::core::Application::part>` resolves a service by its public
interface.
Parts use their manager access for the same typed lookup.
Dependency ordering guarantees that a declared dependency has reached its running state before a dependent part begins
initialization.

The service interface still crosses threads.
Its methods must be thread-safe or enqueue work through the part's event target; dependency ordering does not serialize
ordinary calls after startup.

Combining Parts with Custom Main Behavior
=========================================

Automatic part startup belongs to the base ``Application::main()`` fallback.
An overridden main method, a ``setMainFn()`` callback, or a selected option-module main function must call
``partManager()->start()`` when it needs the registered services.
Such code must also respect asynchronous startup, using callbacks or safe waits from a thread that is not managed by the
part manager.

:doc:`application_parts` explains how to design the interfaces and implementations that make up the graph.
:doc:`detached_application_parts` shows how to operate the same model without application ownership.
