..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application Parts; Designing
    single: ApplicationPart; Lifecycle
    single: ApplicationPart; Dependencies
    single: ApplicationPart; Thread Safety

***************************
Designing Application Parts
***************************

An application part is a one-shot service with a stable identity, declared dependencies, and a dedicated event thread.
This page continues from :doc:`applications_from_parts` and explains how to design service interfaces and
implementations that remain clear across lifecycle and thread boundaries.

Publishing a Stable Service Interface
=====================================

Clients should depend on a small abstract interface rather than the concrete part class.
The interface publishes a stable :cpp:class:`ApplicationPartIdentifier <erbsland::core::ApplicationPartIdentifier>` that
both clients and the implementation use for lookup.

Identifier names are case-sensitive and should follow a reverse-domain form.
They may contain ASCII letters, digits, dots, underscores, and hyphens and have a maximum length of 200 characters.
Managers compare their names, so separately obtained identifier objects with the same name address the same service.

.. erbsland-demo::
    :source: core/ApplicationParts/CatalogStorage.hpp
    :source-sha256: 64b912b47734c4f4e8cd07ab53cf3692960f8eacb2ec54aa54a7046fde58a926

.. code-block:: cpp

    /// Public service interface for the catalog storage part.
    class CatalogStorage {
    public:
        // defaults
        virtual ~CatalogStorage() = default;

    public:
        /// Return the stable identifier shared by clients and the implementation.
        [[nodiscard]] static auto partIdentifier() -> el::ApplicationPartIdentifierPtr {
            static const auto result = el::ApplicationPartIdentifier::create("dev.erbsland.demo.catalog-storage"_el);
            return result;
        }
        /// Describe the prepared storage service.
        [[nodiscard]] virtual auto status() const -> el::String = 0;
    };

.. erbsland-demo-end::

Implementing and Constructing a Part
====================================

:cpp:class:`ApplicationPartWithInterface <erbsland::core::ApplicationPartWithInterface>` combines one public service
interface with the lifecycle supplied by ``ApplicationPart``.
The concrete class provides a static ``create()`` factory so graph preparation can defer construction until every
registration has been validated.

The factory returns the concrete shared pointer type.
Construction should establish ordinary object invariants, while ``initialize()`` performs work that belongs on the
dedicated part thread.

.. erbsland-demo::
    :source: core/ApplicationParts/CatalogStoragePart.hpp
    :source-sha256: 57fcbfa2feb22200ee6cf268d9f4509d50c105f80dd7f7ba16e7b0bb535c5927

.. code-block:: cpp

    /// Concrete application part providing catalog storage.
    class CatalogStoragePart final : public el::ApplicationPartWithInterface<CatalogStorage> {
    public:
        /// Create one storage part when the manager prepares the dependency graph.
        [[nodiscard]] static auto create() -> std::shared_ptr<CatalogStoragePart> {
            return std::make_shared<CatalogStoragePart>();
        }

    public: // implement CatalogStorage
        [[nodiscard]] auto status() const -> el::String override { return "catalog storage is ready"_el; }

    protected: // implement ApplicationPart
        void initialize() override { el::io::printLine("catalog storage initialized"_el); }
        void cleanup() noexcept override { el::io::printLine("catalog storage cleaned up"_el); }
    };

.. erbsland-demo-end::

Declaring Dependencies
======================

A concrete part declares its dependencies through a static ``dependencies()`` method.
The default inherited implementation returns an empty list.
Preparation rejects duplicate identifiers, missing or self dependencies, dependency cycles, invalid factories, and
metadata failures before startup begins.

A dependency reaches ``Running`` before the dependent part's ``initialize()`` starts.
Shutdown reverses that relationship: dependents stop before the services they use.
The server part below can therefore resolve storage during initialization and release it during cleanup.

.. erbsland-demo::
    :source: core/ApplicationParts/CatalogServerPart.hpp
    :source-sha256: 6af2deac53a18464e8fe2925dce9b4ab82da3f0049b031f315cec484527d9716

.. code-block:: cpp

    /// Concrete server part that starts after catalog storage and accesses it through its public interface.
    class CatalogServerPart final : public el::ApplicationPartWithInterface<CatalogServer> {
    public:
        /// Declare the services that must be running before this part starts.
        [[nodiscard]] static auto dependencies() -> el::ApplicationPartIdentifierList {
            return el::ApplicationPartIdentifierList{CatalogStorage::partIdentifier()};
        }
        /// Create one server part when the manager prepares the dependency graph.
        [[nodiscard]] static auto create() -> std::shared_ptr<CatalogServerPart> {
            return std::make_shared<CatalogServerPart>();
        }

    public: // implement CatalogServer
        [[nodiscard]] auto endpoint() const -> el::String override { return "local catalog endpoint"_el; }

    protected: // implement ApplicationPart
        void initialize() override {
            _storage = partManager().part<CatalogStorage>();
            el::io::printLine("catalog server initialized; "_el, _storage->status());
        }
        void cleanup() noexcept override {
            _storage.reset();
            el::io::printLine("catalog server cleaned up"_el);
        }

    private:
        std::shared_ptr<CatalogStorage> _storage;
    };

.. erbsland-demo-end::

Following the Part Lifecycle
============================

Each started part moves through ``Starting``, ``Running``, ``Stopping``, and either ``Stopped`` or ``Failed``.
Its ``initialize()``, ``running()``, ``stopping()``, and ``cleanup()`` hooks execute on its dedicated thread.
The ``automaticStart()`` decision is the exception: the manager evaluates it on the control event source while initial
startup is being planned.

Returning ``false`` from ``automaticStart()`` leaves the part uninitialized during automatic startup.
Automatic dependents remain inactive as well.
An explicit ``start(identifier)`` later starts the requested part and its inactive dependency closure regardless of
those automatic-start choices.

Lifecycles are one-shot.
A part that has stopped or failed cannot be restarted in the same prepared manager.

Sharing Command-Line Configuration
==================================

Part command-line hooks run synchronously on the application's calling thread, not on the part thread.
``registerCommandLineOptions()`` contributes definitions after graph preparation.
``parseCommandLine()`` receives values only after the complete command line has parsed successfully.

These hooks do not parse arguments themselves and they run before part startup.
Use them to copy immutable startup settings into the part, then treat those settings as fixed once the event thread
begins.

Calling Services Across Threads
===============================

A dependency is available before its dependent starts, but ordinary service calls are not automatically synchronized.
A const method that returns immutable state may be safe directly.
Operations that touch event-owned state should post a callback through the service's ``events()`` target or provide
their own locking and completion model.

Do not wait for a manager or part state from the manager control source or from any managed part thread.
Such a wait would prevent the event that satisfies it from running and is rejected with ``LogicError``.

Handling Failures and Shutdown
==============================

An exception from a lifecycle hook or an ordinary part event callback enters the manager's ordered error queue.
The default error action stops the complete graph.
A custom error handler may return ``Continue`` to stop the failed branch and its active dependents while unrelated
branches continue.

``stopping()`` must return promptly.
Its default implementation completes shutdown immediately.
A part with asynchronous shutdown work can return after starting that work and call ``completeStopping()`` from its
event loop when finished.
Only then does the loop exit and ``cleanup()`` run on the dedicated part thread.

The default graceful-shutdown timeout is 30 seconds per part.
After a timeout the manager requests the event loop to quit and records a failure, but it cannot safely interrupt user
code that never returns.
Keep lifecycle hooks and event callbacks non-blocking so shutdown can always make progress.
