..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Event Loops; Application Parts
    single: Application Parts; Initialization
    single: Application Parts; Command-Line Configuration
    single: Application Parts; Asynchronous Shutdown
    single: Application Parts; Event Errors

**************************************
Using Event Loops in Application Parts
**************************************

An application part gives your event-driven code a dedicated thread and an event loop whose lifetime the application
manages.
You set up callbacks, keep the state they use in the part, and let the framework dispatch work and coordinate shutdown.
The important decisions are when to create event sources, which thread may access their state, and whether those sources
need more callbacks before they can close.

This page explains those decisions from command-line configuration through initialization, normal execution, and
shutdown, including what happens when a callback throws.
It builds on :doc:`using_event_loops` and assumes that you already know how to define and register a part, as described
in :doc:`/topics/core/applications_from_parts` and :doc:`/topics/core/application_parts`.

The Part's Event Loop and Lifecycle
===================================

Each started :cpp:class:`ApplicationPart <erbsland::core::ApplicationPart>` has its own event loop and thread.
The framework retains the part, starts that thread, and runs the loop for you.
You customize the part through its lifecycle hooks; you do not need to create a worker thread or run another loop inside
those hooks.

Preparation happens before the part thread exists.
The loop dispatches ``onInitialize()`` to prepare the part.
After that hook returns, the manager publishes the part's ``Running`` state and queues ``onRunning()`` on the part's
loop to begin its work.
During shutdown, it can keep dispatching until the part finishes its asynchronous work.
After the loop exits, cleanup still runs on that same thread.
During a normal run, the part moves through ``Starting``, ``Running``, and ``Stopping``; after its thread finishes, the
manager reports ``Stopped``, or ``Failed`` if a failure was recorded.
The following diagram shows the normal lifecycle of a started part:

.. mermaid::

    sequenceDiagram
        participant App as Application<br/>thread
        participant Manager as Manager<br/>control loop
        participant Part as Part<br/>loop
        Note over App: Prepare parts and<br/>command-line<br/>configuration
        App->>Manager: Start registered<br/>parts
        Manager->>Part: Start loop and dispatch<br/>onInitialize()
        activate Part
        Part->>Manager: Initialization<br/>completed
        Note over Manager: Publish Running
        Manager->>Part: Queue onRunning()
        Note over Part: Begin work and<br/>dispatch callbacks<br/>and source events
        App->>Manager: Request application<br/>shutdown
        Manager->>Part: Queue onStopping()
        Note over Part: Finish any<br/>asynchronous<br/>shutdown work
        Part->>Part: completeStopping()
        Note over Part: Loop exits, then<br/>cleanup() releases<br/>resources
        deactivate Part
        Note over App: Join part threads<br/>before<br/>Application::cleanup()

The loop context and the ability to dispatch another callback are separate things.
An initialization or stopping hook can use the part's event APIs, but another queued callback on that loop must wait
until the hook returns.

.. list-table::
    :header-rows: 1
    :widths: 24 16 30 30

    * - Phase
      - Thread
      - Available state and resources
      - Callback dispatch
    * - Construction and command-line hooks
      - Application
      - Startup configuration; no part event target yet
      - Part loop has not started
    * - ``onInitialize()``
      - Part
      - Parsed configuration and current loop; create event sources here
      - Other callbacks wait until the hook returns
    * - ``onRunning()``
      - Part
      - Initialization completed and ``Running`` published; begin work here
      - Separate queued callback after initialization
    * - Normal event processing
      - Part
      - Retained sources and runtime state
      - One callback at a time
    * - ``onStopping()`` and asynchronous closure
      - Part
      - Existing sources remain available for shutdown
      - Continues after the hook returns, until stopping completes
    * - ``cleanup()``
      - Part
      - Remaining resources to release
      - Loop has exited; no further dispatch

These contexts describe parts managed by the normal application lifecycle.
The :cpp:func:`automaticStart() <erbsland::core::ApplicationPart::automaticStart>` decision runs on the manager's
control target, not on the part thread.
It selects whether a part starts; it is not an event-source setup hook.
A part that has stopped or failed cannot be restarted in the same manager.

Configuration Before Event Processing
=====================================

Command-line handling is a preparation phase, so you can validate settings before opening connections or starting other
asynchronous work.
For an application using the default lifecycle, startup proceeds in this order:

#. :cpp:func:`Application::initialize() <erbsland::core::Application::initialize>` performs application-wide setup.
#. The framework prepares the registered parts, constructing their instances.
#. The application's :cpp:func:`registerCommandLineOptions() <erbsland::core::Application::registerCommandLineOptions>`
   runs, followed by each part's
   :cpp:func:`registerCommandLineOptions() <erbsland::core::ApplicationPart::registerCommandLineOptions>`.
#. :cpp:func:`Application::parseCommandLine() <erbsland::core::Application::parseCommandLine>` parses the complete
   command line.
#. After successful parsing, the application's
   :cpp:func:`handleCommandLineOptions() <erbsland::core::Application::handleCommandLineOptions>` runs, followed by each
   part's :cpp:func:`handleCommandLineOptions() <erbsland::core::ApplicationPart::handleCommandLineOptions>`.
#. The default application main starts automatic parts and runs the main loop that coordinates them.
   Each started part enters :cpp:func:`ApplicationPart::onInitialize() <erbsland::core::ApplicationPart::onInitialize>` on
   its own event thread.

Override the part's option-registration hook to contribute its option definitions to the shared command line.
Its ``handleCommandLineOptions()`` receives the already parsed values: this is where you validate their meaning for the
part and copy the settings into members.
The application and its parts use the same ``handleCommandLineOptions()`` hook for this phase.
Override ``Application::parseCommandLine()`` only when replacing the parsing behavior itself.

Construction, option registration, and option handling all run synchronously on the thread calling
:cpp:func:`Application::run() <erbsland::core::Application::run>`, normally the main thread.
They finish before automatic part threads start.
**You do not need a mutex between a part's command-line handling and its initialization.** Thread startup makes the
configuration stored in ``handleCommandLineOptions()`` available to ``onInitialize()``.
Treat it as fixed startup configuration, or make later changes on its owning event loop.
If another thread changes it after startup, that is a separate concurrent-access problem.

The part's :cpp:func:`loop() <erbsland::core::ApplicationPart::loop>` is unavailable during construction and
command-line handling.
Store configuration there and defer loop-bound resource creation until initialization.
Help, version output, and module overview skip part value handling and startup; a parsing or validation error also
prevents normal startup.
Part cleanup is not called for an instance that never started, so ordinary member ownership must also handle that case.

The automatic-start step belongs to the default application main.
A custom main function or command module must arrange startup explicitly; see
:doc:`/topics/core/applications_from_parts` for that orchestration.

.. erbsland-in-practice-begin:: Validating Settings Before the Part Starts

The :doc:`file-server application </demo_apps/event/file_server>` contributes its directory, port, and timeout to the
application's command line.
Its two hooks define those options, then validate and store the parsed values:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.cpp
    :show: f:ServerPart::registerCommandLineOptions, f:ServerPart::handleCommandLineOptions
    :sha256: 94d497c0e9ea253874be1fc1f265dd559449019a9afd00883c08e6666581b3f1

.. code-block:: cpp

    void ServerPart::registerCommandLineOptions(const el::OptionsPtr &options) {
        options->addOption("root"_el).setRequired().setHelp("Directory to serve on the local machine."_el);
        options->addOption({"--port"_el, "port"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("17680"_el)
            .setValidateTextValue<el::Port>("Invalid port."_el)
            .setHelp("Local HTTP port; zero selects an available port."_el);
        options->addOption({"--timeout"_el, "timeout"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(0)
            .setHelp("Stop after this many seconds; zero runs until interrupted."_el);
    }

    void ServerPart::handleCommandLineOptions(const el::OptionValuesPtr &values) {
        _root = el::Path::fromNativeOrThrow(values->getText("root"_el)).resolveOrThrow(el::PathResolveMode::Physical);
        if (!_root.info().isDirectory()) {
            throw el::ApplicationError{"The root must name a directory."_el};
        }
        _port = el::Port::fromStringOrThrow(values->getText("port"_el));
        _timeout = el::Seconds{values->getInteger("timeout"_el)};
        if (_timeout.isNegative()) {
            throw el::ApplicationError{"The timeout must be non-negative."_el};
        }
    }

.. erbsland-demo-end::

The values are retained in the part's configuration members:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: c:"Configured, physically resolved"(+0,+2)
    :sha256: 9d2a46bee23e3a6102b79b549873c5ecc1e69e24b5168f809b3494e26a741f36

.. code-block:: cpp

    el::Path _root;                      ///< Configured, physically resolved content root.
    el::Port _port;                      ///< Configured loopback listening port.
    el::TimeDelta _timeout;              ///< Optional demo run duration.

.. erbsland-demo-end::

Both hooks run on the application thread before the part starts.
An invalid directory or negative timeout ends startup before the HTTP source is created.
On successful startup, ``onInitialize()`` reads these members on the part thread without a mutex.
The hooks do not open a listener or schedule any event work.

.. erbsland-in-practice-end::

Setting Up Callbacks in Initialization
======================================

``Application::initialize()`` prepares the application before event dispatch, while ``ApplicationPart::onInitialize()``
executes as a callback in the part's event loop.
The application hook runs before command-line parsing and before normal main-loop dispatch.
It is appropriate for application-wide configuration and can queue work for the application's loop, but it does not
provide the part's event context.
The part hook runs later, with parsed settings available and the part's loop already bound as the current loop.
You can create its event sources directly there.

In the part's ``onInitialize()``, obtain the required event API through ``loop()``, create the sources, and connect
their handlers before starting their operations.
Keep owning source pointers in members when the operations must outlive the hook.
Likewise, retain subscription handles for observers that must stay registered.
Returning from initialization destroys local variables; it must not destroy the only owner of an operation whose
callbacks you still need.
:doc:`using_event_apis` explains source creation and subscriptions in more detail.

.. erbsland-in-practice-begin:: Connecting Handlers Before Starting a Source

The file-server part creates and retains its HTTP source in ``onInitialize()``.
This excerpt selects its readiness and error callbacks:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: c:Event source owned, f:onInitialize
    :hide: c:Handle directory-index requests-c:Report listener readiness(+0,-1), c:Complete part shutdown-c:Bind to loopback(+0,-1)
    :sha256: 060eb0b319334cc4fc27b41e546c325e117f659ddb0c50178998f36ac3358588

.. code-block:: cpp

    el::HttpServerPtr _server;           ///< Event source owned by this part's loop.

    void onInitialize() override {
        // The network source belongs to this part's event loop.
        _server = loop()->get<el::Network>().createHttpServer();
        _server->addStaticContentHandler(el::HttpStaticFileHandler::create(_root, "/files"_el));

        // Report listener readiness after the endpoint has been bound.
        _server->events().onListening([this]() -> void {
            el::io::printLine("Browse http://"_el, _server->localEndpoint()->toString(), "/"_el);
            if (_timeout.isPositive()) {
                loop()->invokeAfter(_timeout, []() -> void { el::application().quit(); });
            }
        });
        // Escalate source errors to the part manager.
        _server->events().onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; });

        // Bind to loopback so the demo exposes files only on the local machine.
        _server->start(el::IpEndpoint{el::IpAddress::fromStringOrThrow("127.0.0.1"_el), _port});
    }

.. erbsland-demo-end::

``loop()->get<el::Network>()`` binds the source to this part's loop, and ``_server`` keeps it alive after the hook
returns.
The handlers are connected before ``start()`` begins the asynchronous listen operation.
They can read the configuration prepared by the command-line hooks because it is fixed before thread startup.
The listening callback observes actual readiness; returning from ``onInitialize()`` alone does not establish it.
The timeout is scheduled from that readiness callback, so it measures time spent listening.

.. erbsland-in-practice-end::

Beginning Work in the Running Phase
===================================

After ``onInitialize()`` returns successfully, the manager publishes the part's
:cpp:enum:`Running <erbsland::core::ApplicationPartState>` state and queues
:cpp:func:`onRunning() <erbsland::core::ApplicationPart::onRunning>` as a separate callback on the part's loop.
Override this hook to start the operations prepared during initialization.
The framework supplies the deferred call, so you can begin work directly in ``onRunning()`` without wrapping it in
another ``loop()->invoke(...)``.
Both hooks run with the part's loop as the current loop; their default implementations do nothing.

Callbacks queued during ``onInitialize()`` can execute before ``onRunning()``.
Keep the setup that every callback needs inside ``onInitialize()``, and begin operations in ``onRunning()`` when they
must start after the manager has published ``Running``.
Each hook must return promptly so the loop can dispatch other callbacks.
Waiting inside either hook for a callback on the same loop prevents that callback from running.

``Running`` means initialization completed and the part can begin normal work.
The manager can start dependent parts without waiting for ``onRunning()`` to finish.
Neither that state nor returning from the hook proves that an asynchronous service is ready: starting a connection is
different from receiving its connected notification.
Dependencies that need initialized state can use it once their own initialization begins.
Consumers that need an asynchronous result require an explicit readiness notification from the service.

.. erbsland-in-practice-begin:: Starting Work in the Running Callback

The registered-part template separates source setup from the operations that begin in the running phase:

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/RegisteredPart/WorkPart.hpp
    :show: f:onInitialize, f:onRunning, f:completeWork, f:cleanup
    :sha256: 71f7cefc1085dfa1e3faa30024161787cb6d754957027fa869cff399ed1d3e4e

.. code-block:: cpp

    void onInitialize() override {
        // TODO: Create and retain event sources on this part's loop and connect their handlers.
        // Connect the final successful completion of your work to completeWork().
    }

    void onRunning() override {
        // TODO: Start your business operations after this part enters Running.
        // Report asynchronous service readiness separately if other parts require it.
    }

    void completeWork() { el::Application::instance().quit(); }

    void cleanup() noexcept override {
        // TODO: Release part-owned resources after its loop exits.
        // Initialization may have failed partway.
    }

.. erbsland-demo-end::

Supply the retained sources and handler connections in ``onInitialize()``, then start your operations directly in
``onRunning()``.
Connect their required successful completion to ``completeWork()`` and release the retained resources in ``cleanup()``.
The completion method requests application-wide shutdown; report service readiness separately when another part needs
it.

.. erbsland-in-practice-end::

State Ownership During Callbacks
================================

Event callbacks dispatched by a part's loop execute one at a time on its thread.
They share that thread with ``onInitialize()``, ``onRunning()``, ``onStopping()``, and ``cleanup()``.
When your runtime state is accessed only there, a callback can read and change members without a mutex: no other
callback on that loop runs concurrently with it.
This makes the part a useful owner for both the event sources and the state those sources update.

Other parts and the application thread still run concurrently.
Calling an ordinary C++ method on a part executes that method on the caller's thread.
The method does not move to the part's thread merely because it belongs to an application part.
An operation that touches event-owned state should post work through the part's target using
:cpp:func:`invoke() <erbsland::event::EventTarget::invoke>`, or provide an explicit synchronization contract.
Carry values and ownership sufficient for deferred execution, and coordinate calls with the service's startup and
shutdown.
See :doc:`communicating_between_loops` for request and reply patterns.

A callback capturing ``this`` borrows the part; it does not keep it alive.
The manager retains a started part through its loop execution and cleanup, so handlers owned by that part's sources can
use this pattern when those sources are released during cleanup.
A callback registered on another loop may outlive that sequence and needs its own lifetime arrangement.
:doc:`callback_lifetimes` explains those ownership choices.

Keep each callback and lifecycle hook bounded and responsive.
While one is executing, this part cannot dispatch a timer, a reply, or its stopping hook.
For expensive work, arrange processing elsewhere and deliver its result back to the owning loop, as described in
:doc:`processing_work_on_event_threads`.
Starting a second event loop inside a callback is not a substitute for returning to the framework's loop.

.. erbsland-in-practice-begin:: Reading Part Statistics Through a Public Interface

The file-server application reports how many directory-index requests it has received when shutdown begins.
The application thread reads through ``ServerStatus``, while the HTTP callback updates the count on the part thread.
The public interface promises a thread-safe snapshot:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerStatus.hpp
    :sha256: a884010214eeab5eb6dda5951d1677764e5136284d731f61895ce2d3479c934c

.. code-block:: cpp

    /// Public statistics interface for the file-server part.
    /// Callers may read a snapshot from any thread without accessing the HTTP source.
    class ServerStatus {
    public:
        // defaults
        virtual ~ServerStatus() = default;

    public: // registration
        /// Get the stable identifier shared by the interface and its application part.
        [[nodiscard]] static auto partIdentifier() -> el::ApplicationPartIdentifierPtr {
            static const auto identifier = el::ApplicationPartIdentifier::create("dev.erbsland.demo.file-server"_el);
            return identifier;
        }

    public: // statistics
        /// Get the number of directory-index requests received so far, from any thread.
        [[nodiscard]] virtual auto indexRequestCount() const -> std::size_t = 0;
    };

.. erbsland-demo-end::

``ServerPart`` implements this interface through ``ApplicationPartWithInterface<ServerStatus>``.
Its getter and the private helper called by the directory-index request callback lock the same mutex:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.cpp
    :show: f:ServerPart::indexRequestCount, f:ServerPart::recordIndexRequest
    :sha256: 9c8de1a4b12e0bc260fa5ca3e27bf87322bb931ee99079f748f4016c48600479

.. code-block:: cpp

    auto ServerPart::indexRequestCount() const -> std::size_t {
        const auto lock = std::scoped_lock{_statisticsMutex};
        return _indexRequestCount;
    }

    void ServerPart::recordIndexRequest() {
        const auto lock = std::scoped_lock{_statisticsMutex};
        ++_indexRequestCount;
    }

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: c:Protects the count(+0,+1)
    :sha256: 80aaed29736177cdaa3f9e9b4505c23baa1f3f0260599f56f79608c2f0cc8e92

.. code-block:: cpp

    mutable std::mutex _statisticsMutex; ///< Protects the count for readers on other threads.
    std::size_t _indexRequestCount{0};   ///< Directory-index requests received on the part loop.

.. erbsland-demo-end::

The application retains a state subscription while ``run()`` is active.
Its callback runs on the manager's control target, the application's main loop, and resolves the service by its public
interface:

.. erbsland-demo::
    :source: event/apps/FileServer/main.cpp
    :show: c:Read a statistics snapshot(+0,+8)
    :sha256: d264900e74ea49d1d8896457033afd0f400e905f0e514592135cc593034b848d

.. code-block:: cpp

    // Read a statistics snapshot on the application thread while the part is stopping.
    const auto stateSubscription = app.partManager()->events().addPartStateChanged(
        [&app](el::ApplicationPartIdentifierPtr, const el::ApplicationPartState state) -> void {
            if (state == el::ApplicationPartState::Stopping) {
                const auto status = app.part<ServerStatus>();
                el::io::printLine("Directory-index requests at shutdown: "_el, status->indexRequestCount());
            }
        });
    return app.run();

.. erbsland-demo-end::

The getter runs directly on the application thread; the mutex protects its read against a simultaneous request callback.
Only the counter is shared this way.
The HTTP source and directory configuration remain owned by the part's loop.
Neither lock is held while building a response or printing the snapshot.
The returned value describes that instant: more requests may arrive before closure finishes, so it is not a final
request total or a shutdown-completion signal.

.. erbsland-in-practice-end::

Shutdown and Cleanup
====================

You do not need to override :cpp:func:`onStopping() <erbsland::core::ApplicationPart::onStopping>` just because your
part uses events.
Its default implementation calls
:cpp:func:`completeStopping() <erbsland::core::ApplicationPart::completeStopping>` immediately.
That is sufficient when the part has no remaining operation that requires asynchronous closure or a final callback.
You may still need ``cleanup()`` to release retained sources on their owning thread.

Application-Wide Shutdown
-------------------------

A callback can call :cpp:func:`Application::quit() <erbsland::core::Application::quit>` to request that the whole
application end.
This requests shutdown rather than destroying the parts immediately.
The manager stops dependents before the parts they depend on, leaving those dependencies available while their consumers
finish.
For each running part, the framework calls ``onStopping()`` on that part's own loop.
The application's coordination loop remains available while the parts stop.

Once a part completes stopping, its loop exits, ``cleanup()`` runs, and the framework joins its thread.
The application finishes the part lifecycle and its managed event threads before calling
:cpp:func:`Application::cleanup() <erbsland::core::Application::cleanup>` and returning from ``run()``.
You do not need to join part threads yourself.
Request application shutdown through ``Application::quit()`` so this coordination can take place.
Platform stop requests use the same lifecycle when service integration is enabled; see
:doc:`/topics/core/service_lifecycle`.

.. erbsland-in-practice-begin:: Releasing a Part's Source During Application Shutdown

After the file-server part's loop exits, the framework calls this cleanup hook on the same part thread:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: f:cleanup
    :sha256: 1d4d240711652dba44b7f4dd9ea21f84c43ced2631df3ee4e2903b24c4fa461b

.. code-block:: cpp

    void cleanup() noexcept override {
        if (_server != nullptr) {
            _server->abort();
            _server.reset();
        }
    }

.. erbsland-demo-end::

On a graceful shutdown, the source has already closed.
The same hook also handles runtime failure or partial initialization: it aborts any remaining source and releases the
owning pointer without waiting for a callback.
The framework completes this part's cleanup and joins its thread before calling the application's own ``cleanup()``.

.. erbsland-in-practice-end::

Keeping the Loop Alive for Asynchronous Closure
-----------------------------------------------

Some resources need their loop to continue processing events while they close.
A connection may still have accepted output to send, or an operation may need to acknowledge cancellation.
In that case, override ``onStopping()`` to stop accepting new application work, begin the required closure, and return.
The part remains in :cpp:enum:`Stopping <erbsland::core::ApplicationPartState>` while callbacks finish that work.
Returning from this hook alone does not complete stopping when you override the default implementation.

Call ``completeStopping()`` once no required asynchronous completion remains.
For several operations, keep track of all outstanding completions rather than ending the loop after the first one.
If everything is already finished when ``onStopping()`` runs, call it directly from the hook.
Otherwise, the callback observing the last completion can make that call.

``completeStopping()`` requests the loop to quit.
Already accepted queued work can still drain, but the loop no longer admits new work or polls sources for future events.
It is therefore the end of the asynchronous shutdown phase, not the way to begin closing a source.
Do not wait for closure inside ``onStopping()``: a callback on the same loop cannot run until the hook returns.
The general distinction between closing resources and quitting loops is covered in :doc:`shutting_down_event_work`.

.. erbsland-in-practice-begin:: Returning to the Loop Until Closure Finishes

The file-server part must allow accepted responses to drain before ending its loop.
Its ``onStopping()`` begins that closure and returns:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: f:onStopping
    :sha256: 23cb7fb5cb804740335ace895085e9d42889e44cb2bcb28de04bb3692fbe076c

.. code-block:: cpp

    void onStopping() override {
        // Allow committed responses to drain before ending this part's event loop.
        if (_server == nullptr || _server->state() == el::NetworkSourceState::Closed ||
            _server->state() == el::NetworkSourceState::Failed) {
            completeStopping();
        } else {
            _server->close();
        }
    }

.. erbsland-demo-end::

While ``_server->close()`` finishes asynchronously, the part keeps dispatching events.
The final handler installed in ``onInitialize()`` supplies the completion signal:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: c:Complete part shutdown(+0,+5)
    :sha256: 702e6f90cc779be69b5c96608c739b986458789fcad17ba0757342f5d4522532

.. code-block:: cpp

    // Complete part shutdown after the source has closed.
    _server->events().onFinal([this]() -> void {
        if (state() == el::ApplicationPartState::Stopping) {
            completeStopping();
        }
    });

.. erbsland-demo-end::

The retained ``_server`` keeps the source alive until this callback runs and cleanup can release it.
If the source is already absent, closed, or failed, ``onStopping()`` calls ``completeStopping()`` immediately.
Otherwise, the final callback makes that call after closure finishes.
This is asynchronous waiting: ``onStopping()`` does not block the thread or wait on a condition variable.

.. erbsland-in-practice-end::

Releasing Resources After Dispatch
----------------------------------

:cpp:func:`ApplicationPart::cleanup() <erbsland::core::ApplicationPart::cleanup>` runs after the part's loop has exited,
on the part thread but outside active dispatch.
Release event sources and subscriptions there, using synchronous abort or release operations when necessary.
It is too late to start another asynchronous close and wait for its callback.
The hook is ``noexcept`` and must not let an exception escape.

Cleanup also runs when a handled runtime failure interrupts initialization or event processing.
Make it safe for partially initialized state, such as a source pointer that was never assigned or an operation that
never reached its final notification.
Do not rely on ``onStopping()`` or a source's final callback having run first.
Keeping owned resources in members allows both explicit cleanup on the part thread and ordinary destruction when an
instance never starts.

The framework bounds graceful stopping with
:cpp:func:`shutdownTimeout() <erbsland::core::ApplicationPart::shutdownTimeout>`, which defaults to 30 seconds per part.
Override it if your part needs a different graceful-shutdown allowance.
Expiry records a failure and requests loop termination, but it cannot forcibly interrupt C++ code that never returns.
A blocked callback can therefore still prevent shutdown from completing.

Unhandled Event Exceptions
==========================

An event source may report an operation error through a dedicated error callback.
That notification lets your code decide whether to recover, finish one request with an error, or fail the part.
Handling it and returning normally lets the part continue processing events.
Throwing from that handler instead turns the operation error into an exception escaping an event callback.

When a :cpp:class:`RuntimeError <erbsland::err::RuntimeError>` or a derived exception escapes a part callback, the
framework records it with the part manager and stops dispatch on the failed part's loop.
The failed callback is not retried, and its previous state changes are not undone.
The part then runs ``cleanup()`` on its own thread.
The same boundary handles these exceptions from ``onInitialize()``, ``onRunning()``, and ``onStopping()``.

By default, the manager also shuts down the other parts in dependency order.
The error reaches ``Application::run()``, which completes application cleanup, prints a diagnostic, and returns a
failure exit code.
An :cpp:class:`ApplicationError <erbsland::core::ApplicationError>` supplies its own exit code.
You do not need to install a separate loop error handler to obtain this application-level failure behavior.
For an error that should remain local to one operation, handle it before it escapes the callback and arrange that
operation's completion yourself.

The failed part does not get an opportunity to finish its usual asynchronous shutdown after dispatch stops.
Its ``onStopping()`` hook or a source's final notification may never run.
This is why indispensable resource release belongs in ``cleanup()`` rather than solely in a successful completion
callback.
Custom manager policies can allow unrelated parts to continue while the failed part and its dependents stop; see
:doc:`/topics/core/application_parts` for that choice.

This handling applies to ``RuntimeError`` and its derived types, not arbitrary C++ exceptions.
A :cpp:class:`LogicFailure <erbsland::err::LogicFailure>` or another exception outside that hierarchy escaping the part
thread terminates the process.
It does not become a normal failure return from ``Application::run()``, and orderly cleanup is not guaranteed.
An exception escaping ``cleanup() noexcept`` also terminates the process.
See :doc:`handling_event_errors` for the wider distinction between operation errors and callback failures.
