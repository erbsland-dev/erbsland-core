..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Event Loops; Application Parts
    single: Application Parts; Event Sources
    single: Application Parts; Asynchronous Shutdown

**************************************
Using Event Loops in Application Parts
**************************************

A service often needs to keep resources alive, respond to incoming work, and finish outstanding operations before it
stops.
An application part gives that service its own event loop and connects its lifetime to the rest of the application.
You can keep its event sources and mutable state together on one thread.

The framework runs the part's initialization, callbacks, stopping hook, and cleanup on its dedicated thread.
Keeping mutable service state on that thread lets callbacks work with it in sequence, without a mutex between those
callbacks.
Other threads communicate with the service by posting work to its target; an ordinary C++ method call does not move to
the part's thread automatically.

This page explains that execution model, when event sources can be created, and how a part keeps dispatching until its
asynchronous shutdown is complete.
The file-server demo provides concrete examples of the same mechanics you would use in another long-lived service.
It builds on :doc:`using_event_loops` and focuses on the event behavior of a part.
:doc:`/topics/core/applications_from_parts` covers application composition, while
:doc:`/topics/core/application_parts` explains service interfaces and dependencies.

Registration and Automatic Startup
==================================

:cpp:func:`registerPart() <erbsland::core::Application::registerPart>` adds a part type before the application runs.
During startup, the framework constructs the part and prepares its command-line options.
After successful parsing, the default application main starts automatic parts and runs the main loop for coordination.
Each started part gets a separate event-loop thread; its ``automaticStart()`` decision and dependencies determine
whether it starts during this initial phase.

The framework retains the part while its lifecycle is active and runs its loop.
Inside the part, you implement lifecycle hooks rather than start another event thread or call ``runEventLoop()``.
The file-server application registers one such part to publish a local directory over HTTP:

.. erbsland-demo::
    :source: event/FileServer/main.cpp
    :exec: event/file_server --port 0 --timeout 1 {directory:root}
    :show-cmd-line:
    :sha256: 66240f3be04ef115741e8afedc41dccd1f4ff9c79ce7db9ff1f900c40afeda9e

.. code-block:: cpp

    /// Register an application part and let the default main manage its event thread and shutdown.
    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.info().setApplicationName("File Server"_el);
        app.enableServiceLifecycle();
        app.registerPart<ServerPart>();
        return app.run();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/file_server --port 0 --timeout 1 root

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBrowse http://127.0.0.1:51380/␛[0m

.. erbsland-demo-end::

The call to ``enableServiceLifecycle()`` connects this application's shutdown to its platform service lifecycle.
The captured run uses an available local port and quits after one second; its port number varies between runs.

For an interactive run, start ``file_server --port 17680 doc/topics/event`` and visit ``http://127.0.0.1:17680/`` in
your browser.
The server binds to loopback, and its page links to files beneath the selected directory.

Configuration and Event-Owned Resources
=======================================

In an application-managed part, command-line hooks run on the application thread before the part thread starts.
Use them to validate input and store configuration values.
The event target is not available before startup, so defer event-source creation to ``initialize()``.
A constructor or command-line hook cannot provide that execution context.

Once callbacks start, configuration and runtime resources belong to different phases of the same service.
Configuration prepared before thread startup can be read by the part; mutable runtime state should be accessed only on
its loop unless you add explicit synchronization.

The file server's ``parseCommandLine()`` validates its root directory, port, and optional timeout.
Those settings and the server pointer are members of ``ServerPart``:

.. erbsland-demo::
    :source: event/FileServer/ServerPart.hpp
    :show: c:"Configured, physically resolved"(+0,+3)
    :sha256: 352490a4bcaac9844d9743c213e6d56c3a02747766b4240033ec1236d7ddf446

.. code-block:: cpp

    el::Path _root;            ///< Configured, physically resolved content root.
    el::Port _port;            ///< Configured loopback listening port.
    el::TimeDelta _timeout;    ///< Optional demo run duration.
    el::HttpServerPtr _server; ///< Event source owned by this part's loop.

.. erbsland-demo-end::

The configuration becomes fixed before the event thread begins using it.
The ``_server`` member will keep the event source alive after setup returns.
A temporary local server pointer in the setup function would not provide that lifetime.

Initialization and Asynchronous Readiness
=========================================

``initialize()`` runs on the part's thread with its event loop bound as the current loop.
:cpp:func:`events() <erbsland::core::ApplicationPart::events>` returns the target for that loop.
Use this context to create event sources, retain their owning pointers in members, connect handlers, and start their
operations.
Timers and invocations scheduled through this target also run on the part's loop.

After ``initialize()`` and ``running()`` return, the framework reports the part as running and dispatches queued work.
That lifecycle state does not mean that asynchronous operations started during initialization are already ready.
Use each source's readiness notification before depending on its result; do not block initialization waiting for a
callback that requires this loop to dispatch.

This excerpt keeps the file server's lifecycle handlers visible and omits its request handler.
The server is created through the part's target, and ``onListening()`` announces actual network readiness:

.. erbsland-demo::
    :source: event/FileServer/ServerPart.hpp
    :show: f:initialize
    :hide: c:"The network source belongs"(-4,+14)
    :sha256: c3128aefefdb410566f1ac846fb09b0a0ef2c93e04de320649577700d6478c70

.. code-block:: cpp

    void initialize() override {
        // The network source belongs to this part's event loop.
        _server = events()->get<el::Network>().createHttpServer();
        _server->addStaticContentHandler(el::HttpStaticFileHandler::create(_root, "/files"_el));
        _server->events()

            .onListening([this]() -> void {
                el::io::printLine("Browse http://"_el, _server->localEndpoint()->toString(), "/"_el);
                if (_timeout.isPositive()) {
                    events()->invokeAfter(_timeout, []() -> void { el::application().quit(); });
                }
            })
            .onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; })
            .onFinal([this]() -> void {
                if (state() == el::ApplicationPartState::Stopping) {
                    completeStopping();
                }
            });
        // Bind to loopback so the demo exposes files only on the local machine.
        _server->start(el::IpEndpoint{el::IpAddress::fromStringOrThrow("127.0.0.1"_el), _port});
    }

.. erbsland-demo-end::

``start()`` begins the asynchronous listen operation and returns.
The stored server owns its handlers; the final handler participates in the shutdown described below.
HTTP routing and response construction are covered in :doc:`/topics/network/using-http-servers`.

Once listening begins, this demo schedules its optional timeout through the part's target.
That timeout callback also runs on the part thread and requests application-wide shutdown.
``invokeAfter()`` retains the queued callback without a stored timer handle; a repeating timer created with
``createTimer()`` would need an owning member, just as in :doc:`scheduling_work`.

Callback Execution and State Ownership
======================================

Callbacks from sources created on the part's loop execute one at a time on the part thread.
Lifecycle hooks that access runtime resources use that same thread.
If all mutations stay there, no two callbacks can change the part's state concurrently, and those members need no
application mutex.
This is a rule for how your service uses its state, not automatic protection applied to every member of an
``ApplicationPart``.

In the file server, the request, listening, error, and final handlers follow that rule.
Each can access the configuration and server without competing with another callback on the same loop.

The handlers capture ``this``.
That is safe here because the framework retains the part through shutdown, its loop finishes before cleanup, and cleanup
releases the server that owns those handlers.
The callbacks do not keep the part alive through a shared-pointer cycle.
:doc:`callback_lifetimes` explains how this changes when a callback recipient has an independent lifetime.

Cross-Thread Service Calls
==========================

The main thread and other parts still run concurrently.
An ordinary call to a method on ``ServerPart`` runs on the caller's thread; owning an event loop does not redirect C++
method calls.
A service interface that changes event-owned state should queue that operation through the part's target with
:cpp:func:`invoke() <erbsland::event::Events::invoke>`, carrying values and ownership sufficient for deferred execution.
Its caller must also coordinate with the service's startup and shutdown.
:doc:`/topics/core/application_parts` covers the service-interface boundary, and
:doc:`processing_work_on_event_threads` demonstrates value transfer between loops.

Keep callbacks short enough for the service to remain responsive.
In this demo, directory-index creation is synchronous, so it delays other callbacks on the part while it runs.
For expensive processing, delegate work and deliver the completed result back to the owning loop.

Asynchronous Shutdown
=====================

The part manager calls :cpp:func:`stopping() <erbsland::core::ApplicationPart::stopping>` on the part's thread when
shutdown begins.
The default implementation completes stopping immediately.
Override it when owned resources need further event processing to finish, such as a connection draining pending output.
Begin that close operation and return so the loop can dispatch its completion callback.

Call :cpp:func:`completeStopping() <erbsland::core::ApplicationPart::completeStopping>` when no required asynchronous
shutdown work remains.
It requests termination of the part's loop; cleanup follows after dispatch ends.
For multiple sources, track all outstanding close operations and complete stopping only when all have finished.
If no asynchronous work remains, complete stopping directly from the hook.

In the file-server demo, the timeout requests :cpp:func:`quit() <erbsland::core::Application::quit>`, and the
application stops its parts before closing the remaining event system.
The server may still have committed responses to deliver, so ``stopping()`` begins closing it:

.. erbsland-demo::
    :source: event/FileServer/ServerPart.hpp
    :show: f:stopping
    :sha256: 1eee5ee8bcee081fd561dd4670ac37765a54c602977a434047b6dc0bd73a3490

.. code-block:: cpp

    void stopping() override {
        // Allow committed responses to drain before ending this part's event loop.
        if (_server == nullptr || _server->state() == el::NetworkSourceState::Closed ||
            _server->state() == el::NetworkSourceState::Failed) {
            completeStopping();
        } else {
            _server->close();
        }
    }

.. erbsland-demo-end::

On the asynchronous close path, returning from this hook does not complete the part's shutdown.
The loop continues dispatching while the server closes.
Once its final notification arrives, the ``onFinal()`` handler installed during initialization checks that the part is
stopping and calls :cpp:func:`completeStopping() <erbsland::core::ApplicationPart::completeStopping>`.
That is the signal that this part no longer needs event processing.

.. mermaid::

    flowchart TD
        Stop["stopping(): ask the server to close"]
        Dispatch["Return to the loop;<br/>continue processing server events"]
        Final["onFinal(): completeStopping()"]
        Cleanup["Loop exits; cleanup() releases resources"]
        Stop --> Dispatch --> Final --> Cleanup

If the server is already closed, failed, or absent, there is nothing left to drain, and ``stopping()`` completes
immediately instead.
The default implementation of ``stopping()`` also completes immediately; this override deliberately keeps the loop alive
for the server's asynchronous close.

Blocking in ``stopping()`` until a final callback arrives would prevent that callback from running on the same thread.
Calling ``completeStopping()`` before the server finishes would remove the event processing it still needs.
The begin-close, return, complete-later sequence avoids both problems.

Cleanup and Failure Paths
=========================

After the loop exits, :cpp:func:`cleanup() <erbsland::core::ApplicationPart::cleanup>` runs on the same part thread.
It releases resources without starting new asynchronous work:

.. erbsland-demo::
    :source: event/FileServer/ServerPart.hpp
    :show: f:cleanup
    :sha256: 6d6472fffdd87a72a41614fa3bd5f6d5fe14b50dd6efd55e844fdf7549b04992

.. code-block:: cpp

    void cleanup() noexcept override {
        if (_server != nullptr) {
            _server->abort();
            _server.reset();
        }
    }

.. erbsland-demo-end::

On a normal shutdown, the server has already finished closing.
``abort()`` also provides a fallback when an error prevents that normal sequence from completing.
Resetting ``_server`` releases the source and its captured handlers.
The null check handles initialization that failed before a server was stored.

A final source callback is not a substitute for cleanup.
Here, ``onError()`` throws a ``NetworkError`` so a server failure reaches the part manager; another callback or
lifecycle hook can throw as well.
The default manager policy stops the graph, and cleanup must remain safe after partial initialization or failed work.
Keep it non-throwing and use resource-owning members for unconditional release.

The manager also limits graceful shutdown; the default timeout is 30 seconds per part.
After a timeout it requests loop termination and records a failure, but it cannot interrupt a callback that never
returns.
This makes prompt returns from callbacks and lifecycle hooks part of the shutdown design.
:doc:`/topics/core/application_parts` covers dependencies, failure handling, and lifecycle configuration beyond this
single-service example.
:doc:`shutting_down_event_work` explains the general shutdown sequence, including queue draining, operation
cancellation, and thread joins outside the part framework.
