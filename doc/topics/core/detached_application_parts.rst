..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application Parts; Detached Manager
    single: ApplicationPartManager; Detached
    single: ApplicationPartManager; Waiting

***************************************
Running Application Parts Independently
***************************************

The application-part framework can manage a service graph without an
:cpp:class:`Application <erbsland::core::Application>` owner.
A detached :cpp:class:`ApplicationPartManager <erbsland::core::ApplicationPartManager>` is useful in an embedded host, a
test harness, or a process whose outer lifecycle is controlled by another framework.

Owning the Lifecycle Explicitly
===============================

The caller registers parts, prepares the graph, starts it, waits for startup, uses prepared service interfaces, and
finally requests shutdown.
Creating a manager without an event target gives it a dedicated control event thread in addition to the event thread of
every started part.

.. erbsland-demo::
    :source: core/ApplicationParts/DetachedMain.cpp
    :exec: core/core_detached_parts
    :source-sha256: 668586eea6863f8111e81bfa38a135ab7720a55d56e9c275908181343f85bb48

.. code-block:: cpp

    /// Run application parts independently when no `Application` lifecycle should own them.
    ///
    /// A detached manager owns its control event thread, while every started part receives its own event thread.
    /// The caller explicitly prepares, starts, waits for, uses, and stops the service graph.
    auto main() -> int {
        auto manager = el::ApplicationPartManager::create();
        manager->registerPart<CatalogStoragePart>();
        manager->registerPart<CatalogServerPart>();
        manager->prepare();
        manager->start();
        if (!manager->waitForRunning()) {
            if (manager->hasError()) {
                std::rethrow_exception(manager->takeError());
            }
            return el::ExitCode::failure().toRawValue();
        }

        const auto server = manager->part<CatalogServer>();
        el::io::printLine("using "_el, server->endpoint());

        manager->stop();
        if (!manager->waitForStopped() && manager->hasError()) {
            std::rethrow_exception(manager->takeError());
        }
        return el::ExitCode::success().toRawValue();
    }

.. erbsland-ansi::
    :escape-char: ␛

    catalog storage initialized
    catalog server initialized; catalog storage is ready
    using local catalog endpoint
    catalog server cleaned up
    catalog storage cleaned up

.. erbsland-demo-end::

Preparing Before Startup
========================

``registerPart<T>()`` stores the metadata and factory for a concrete part.
``prepare()`` validates the complete dependency graph and constructs every part before changing the manager to
``Ready``.
Typed ``part<T>()`` lookup is available after preparation even if the selected part has not started.

Command-line integration is optional in a detached host.
When it is needed, call ``registerCommandLineOptions()`` after preparation and ``parseCommandLine()`` after successful
parsing, before startup.

Waiting from an Owning Thread
=============================

``start()`` and ``stop()`` are asynchronous requests.
``waitForRunning()`` returns ``true`` only when initial startup settles in ``Running``.
``waitForStopped()`` distinguishes a clean ``Stopped`` result from ``Failed``.
When a wait reports failure, ``hasError()`` and ``takeError()`` expose queued exceptions in their recorded order.

Wait only from an external owning thread.
The manager rejects waits from its control source and from managed part threads because those threads must keep
dispatching lifecycle work.

Starting and Stopping One Branch
================================

``start(identifier)`` starts an inactive part together with the dependencies it needs.
``stop(identifier)`` first stops every active dependent that relies on the selected part.
Independent branches continue running unless the failure policy or caller requests a complete stop.

The matching per-part wait functions allow an external owner to observe these targeted transitions.
Because each lifecycle is one-shot, targeted start is for a prepared but previously inactive part, not for restarting a
stopped service.

Supplying an Existing Event Target
==================================

``ApplicationPartManager::create(events)`` places manager control callbacks on an existing event target.
The caller retains ownership of that target and must keep it dispatching until the manager reaches ``Stopped`` or
``Failed``.

This form integrates manager decisions into another event system, but it does not move part work onto that same thread.
Every started part still owns its dedicated event loop, and its public interface still needs a deliberate cross-thread
contract as described in :doc:`application_parts`.
