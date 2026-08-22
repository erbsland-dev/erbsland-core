.. index::
    single: Application Parts; Using
    single: Dependency Graph; Application Parts

********************************
Building Applications from Parts
********************************

Application parts let you divide a process into services that have explicit lifecycle dependencies without coupling them
to one application subclass.
Each running part owns an event loop and thread, while one control event source serializes graph decisions.
This page explains how to define parts, start them in a standalone manager or an application, forward command-line data,
and handle orderly failure and shutdown.

Defining Stable Service Interfaces
==================================

A service interface publishes a stable identifier.
Its name is case-sensitive, contains at most 200 ASCII letters, digits, dots, underscores, or hyphens, and should use a
reverse-domain form.
Name equality is authoritative; object addresses are never part of lookup semantics.

.. code-block:: cpp

    #include <erbsland/core/ApplicationPartIdentifier.hpp>

    class CacheService {
    public:
        virtual ~CacheService() = default;

        static auto partIdentifier() -> el::core::ApplicationPartIdentifierPtr {
            static const auto result = el::core::ApplicationPartIdentifier::create(
                "com.example.cache"_el);
            return result;
        }

        virtual void clear() = 0;
    };

The shared pointer is intentionally non-const.
Its concrete implementation can cache a manager token and compact numeric identifier, but the public identifier
interface exposes only ``name()`` and ``toString()``.
Reusing one identifier with multiple managers remains correct because every manager validates the cache against its own
token and falls back to the name.

Declaring a Part and Its Dependencies
=====================================

Derive an implementation from ``ApplicationPartWithInterface`` when clients need a typed service.
Static metadata is resolved during ``prepare()``, and construction is deferred until the complete graph passes
validation.

.. code-block:: cpp

    #include <erbsland/core/ApplicationPartManager.hpp>

    class CachePart final : public el::core::ApplicationPartWithInterface<CacheService> {
    public:
        static auto create() -> std::shared_ptr<CachePart> {
            return std::make_shared<CachePart>();
        }

        void clear() override {
            events()->invoke([this]() { entries.clear(); });
        }

    protected:
        void initialize() override {
            // Open files or establish resources on this part's thread.
        }

        void cleanup() noexcept override {
            entries.clear();
        }

    private:
        std::vector<Entry> entries;
    };

A concrete class may declare dependencies by returning an ``ApplicationPartIdentifierList``.
The inherited default is an empty list.

.. code-block:: cpp

    static auto dependencies() -> el::core::ApplicationPartIdentifierList {
        return el::core::ApplicationPartIdentifierList{
            CacheService::partIdentifier(), MetricsService::partIdentifier()};
    }

Preparation rejects duplicate names, missing or self dependencies, cycles, null factories, and metadata failures before
it exposes a ready graph.
It then constructs parts in registration order and makes them available through ``part()`` even before startup.

Running a Detached Manager
==========================

With no argument, the manager creates and owns a control event thread.
An injected ``Events`` target remains owned by the caller and must keep dispatching until the manager reaches
``Stopped`` or ``Failed``.

.. code-block:: cpp

    auto manager = el::core::ApplicationPartManager::create();
    manager->registerPart<CachePart>();
    manager->registerPart<ApiPart>();
    manager->prepare();
    manager->start();

    if (!manager->waitForRunning()) {
        std::rethrow_exception(manager->takeError());
    }

    auto cache = manager->part<CacheService>();
    cache->clear();

    manager->stop();
    manager->waitForStopped();

Automatic startup evaluates ``automaticStart()`` at most once during the initial ``Starting`` phase.
A false result leaves that part uninitialized and prevents automatic dependents from being evaluated.
A later explicit ``start(identifier)`` ignores these opt-outs and recursively starts the complete inactive dependency
closure.

Shutdown follows the reverse graph.
Stopping a single part first stops all active transitive dependents; stopping the manager processes independent
reverse-dependency branches concurrently.
Lifecycles are one-shot, so stopped and failed parts cannot restart.

Forwarding Command-Line Data
============================

After preparation, ``registerCommandLineOptions()`` and ``parseCommandLine()`` synchronously visit parts in registration
order.
These calls do not parse arguments themselves.
They let the application's regular options framework share its definitions and successful ``OptionValues`` with each
part.
Exceptions propagate synchronously to the caller.

Integrating Parts into Application
==================================

``Application::registerPart<T>()`` lazily creates a manager controlled by ``Application::events()``.
``run()`` prepares the graph after application initialization, forwards option registration and successful parsing, and
coordinates part shutdown before the main event system exits.

The base ``Application::main()`` starts automatic parts only when it falls back to the application event loop.
An option module main, a ``setMainFn()`` callback, or an overridden ``main()`` must explicitly call
``partManager()->start()`` when it needs the registered parts.

.. code-block:: cpp

    class ServerApplication final : public el::core::Application {
    protected:
        void initialize() override {
            registerPart<CachePart>();
            registerPart<ApiPart>();
        }
    };

Calling ``Application::quit()`` records only the first exit code.
When parts are active, it first requests reverse-order part shutdown and waits for the manager's terminal callback
before quitting the application event loop and generic managed event threads.

Handling Failures and Asynchronous Shutdown
===========================================

Exceptions from ``initialize()``, ``running()``, ``stopping()``, or an ordinary part event callback enter the manager's
ordered error queue.
The default error action is ``StopAll``.
A custom handler may return ``Continue`` to stop the failed part and its dependents while preserving unrelated branches.
Throwing error or state callbacks force a full shutdown; the original part error remains first in the queue.

``stopping()`` must return promptly.
The default calls ``completeStopping()`` immediately, while asynchronous parts may start shutdown work and call it later
from their event loop.
After completion, dispatch ends and ``cleanup()`` runs on the same dedicated part thread.

The default shutdown timeout is 30 seconds per part.
Expiry requests the part loop to quit, records a ``RuntimeError``, and publishes ``Failed`` only after the worker exits
and cleanup completes.
A timeout cannot interrupt blocking user code: C++ threads cannot be killed safely, so a callback that never returns can
still prevent shutdown.

Do not call a manager wait function from its control thread or any managed part thread.
Such a wait would deadlock and is rejected with ``LogicError``.
Cross-thread service methods need their own synchronization or should enqueue work on the service's ``events()`` target;
dependency availability does not make arbitrary interface calls thread-safe.
