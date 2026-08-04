.. index::
    single: Host Lookup
    single: DNS
    single: Network; Resolving Hosts

******************************
Resolving Hosts Asynchronously
******************************

Resolving a host name can take longer than one event-loop turn, especially when the system resolver needs to contact a
DNS server.
Blocking the event loop during that work would also delay unrelated application tasks.

This page shows you how to use :cpp:class:`HostLookup <erbsland::network::HostLookup>` to resolve a host without
blocking the event loop.
You will learn how to receive every address, handle failures, choose the owning event loop, cancel work, and let the
lookup lifetime follow the object that needs its result.

If you are not yet familiar with :cpp:class:`Host <erbsland::network::Host>`,
:cpp:class:`HostName <erbsland::network::HostName>`, and :cpp:class:`IpAddress <erbsland::network::IpAddress>`, start
with :doc:`working-with-ip-addresses-and-hostnames`.

Start with a Host and Wait for the Result
=========================================

Create the lookup through the :cpp:class:`Network <erbsland::network::Network>` facade belonging to the event loop that
should receive its callbacks.
Configure the callbacks through ``events()`` before passing a :cpp:class:`Host <erbsland::network::Host>` to
``start()``.

The lookup owns its callbacks and the stable
:cpp:class:`HostLookupEventEditor <erbsland::network::HostLookupEventEditor>` returned by ``events()``.
Keep the :cpp:type:`HostLookupPtr <erbsland::network::HostLookupPtr>` alive until ``onFinal()`` runs.
The final callback is the natural place to release the lookup or start the next operation.

.. erbsland-demo::
    :source: network/HostLookupDomain/main.cpp
    :source-sha256: ffb08dee5209922a5c42add4094edc57aa2375caff0f0408ec6bd57d9ecca607

.. code-block:: cpp

    /// Resolve a public host name without blocking the event loop.
    /// Keep the lookup alive until the final event runs; the lookup owns its stable event editor.
    /// A host can resolve to several addresses, so successful code processes the complete result list.
    void resolveDocumentationHost() {
        const auto events = el::application().events();
        lookup = events->get<el::Network>().createHostLookup();
        lookup->events()
            .onResolved([](const el::List<el::IpAddress> &addresses) -> void {
                el::stdOut()->printLine("core.erbsland.dev resolved to:"_el);
                for (const auto &address : addresses) {
                    el::stdOut()->printLine("  "_el, address);
                }
            })
            .onError([](const el::NetworkErrorContext &error) -> void {
                el::stdErr()->printLine("Lookup failed: "_el, error.title());
                el::application().quit(el::ExitCode::failure());
            })
            .onFinal([]() -> void { el::application().quit(); });
        lookup->start(el::Host::fromStringOrThrow("core.erbsland.dev"_el));
    }

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableTerminal();
        app.events()->invoke(resolveDocumentationHost);
        return app.run();
    }

.. erbsland-demo-end::

``onResolved()`` receives a list because one host name can refer to several IPv4 and IPv6 addresses.
Process the whole list unless your application has a deliberate address-selection policy.
The order comes from the platform resolver, so it may differ between systems or change when DNS records change.

Calling ``start()`` changes the lookup from
:cpp:enumerator:`NetworkSourceState::Inactive <erbsland::network::NetworkSourceState::Inactive>` to
:cpp:enumerator:`NetworkSourceState::Starting <erbsland::network::NetworkSourceState::Starting>`.
A handler always runs in a later event-loop dispatch, including when the input is already a numeric address.
The lookup has the terminal ``Closed`` or ``Failed`` state while the corresponding result handler runs.
It returns to ``Inactive`` and clears ``host()`` before ``onFinal()`` runs.
That ordering gives cleanup code a predictable point at which the lookup is ready to use again.

Give Slow Lookups a Deadline
============================

Each ``start()`` call accepts :cpp:class:`HostLookupOptions <erbsland::network::HostLookupOptions>`.
The defaults give the complete operation a ten-second deadline, allow two native resolver attempts, and wait 100
milliseconds before the second attempt.
Only a temporary failure reported by the native resolver is retried.
A missing host, an empty address list, and other permanent resolver errors fail immediately.

The deadline covers time spent waiting for a resolver worker, performing native attempts, and waiting between retries.
The platform's blocking resolver call cannot be interrupted portably.
If that call outlives the deadline, ``onError()`` and ``onFinal()`` run when the deadline expires while the worker
finishes in the background.
The late result is discarded.
As with every event timeout, the deadline can be delivered only while the owner event loop remains responsive.

.. code-block:: cpp

    auto options = el::HostLookupOptions{};
    options.setTimeout(el::Seconds{5})
        .setMaximumAttempts(el::ItemCount{3U})
        .setRetryDelay(el::Milliseconds{250});
    lookup->start(el::Host::fromStringOrThrow("core.erbsland.dev"_el), options);

The timeout and maximum attempt count must be positive, and the retry delay must not be negative.
Invalid options throw synchronously from ``start()`` so configuration mistakes are visible at the call site.

Turn Failures into Useful Diagnostics
=====================================

Resolver failures arrive through ``onError()`` as a
:cpp:class:`NetworkErrorContext <erbsland::network::NetworkErrorContext>`.
The context contains a machine-readable
:cpp:enum:`NetworkErrorReason <erbsland::network::NetworkErrorReason>`, the requested host, and native resolver
details when they are available.
Use the reason when code needs to choose a recovery strategy, and use the title or description when reporting the
failure to a person.

If your event-driven application has a standard diagnostic boundary, throw
:cpp:class:`NetworkError <erbsland::network::NetworkError>` from the handler to preserve the structured context.

.. erbsland-demo::
    :source: network/HostLookupFailure/main.cpp
    :source-sha256: 63c69f2f0c7907d830443af51f0b80d1c1d07682b85bf12719f3d3f163dc3308

.. code-block:: cpp

    /// Resolver failures arrive through `onError()` as a structured context.
    /// Throwing `NetworkError` transfers that context to the application's standard diagnostic boundary.
    void resolveMissingHost() {
        const auto events = el::application().events();
        lookupData.lookup = events->get<el::Network>().createHostLookup();
        lookupData.lookup->events().onError(
            [](const el::NetworkErrorContext &context) -> void { throw el::NetworkError{context}; });
        lookupData.lookup->start(el::Host::fromStringOrThrow("stelling.invalid"_el));
    }

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableTerminal();
        app.events()->invoke(resolveMissingHost);
        return app.run();
    }

.. erbsland-demo-end::

The error handler observes the ``Failed`` state.
The lookup then returns to ``Inactive`` before ``onFinal()`` runs.
Invalid API use, such as starting another operation while the current one is active, throws synchronously instead of
calling ``onError()``.
If ``onResolved()`` or ``onError()`` throws, the lookup still returns to ``Inactive`` but skips ``onFinal()``.
The exception continues through the event loop.

Keep Callbacks with Their Owner Event Loop
==========================================

Every lookup belongs to the event loop whose :cpp:class:`Network <erbsland::network::Network>` facade created it.
Its handlers run in that loop, so related application state can stay confined to a main loop, worker loop, or other
dedicated event loop.

Create the lookup and call ``events()`` and ``start()`` from its owner loop.
If another loop should own the operation, post the complete setup to that loop with ``invoke()``.
The following demo creates a managed event thread and verifies that the lookup handler runs there.

.. erbsland-demo::
    :source: network/HostLookupEventThread/main.cpp
    :exec: network/host_lookup_event_thread
    :source-sha256: 5c63fcfb423d7d910176a57be3f18fa3db6394440db4ef486f1714699198771f

.. code-block:: cpp

    /// Create and start a lookup in the event loop that shall own it.
    /// Its handlers run in that same loop, so other application state can stay confined to the chosen event thread.
    void resolveInWorkerLoop() {
        const auto workerEvents = el::currentEvents();
        lookupData.lookup = workerEvents->get<el::Network>().createHostLookup();
        lookupData.lookup->events()
            .onResolved([workerEvents](const el::List<el::IpAddress> &) -> void {
                const auto runsInWorker = el::currentEvents() == workerEvents;
                el::stdOut()->printLine("Handler runs in worker event loop: "_el, runsInWorker);
            })
            .onFinal([]() -> void { el::application().quit(); });
        lookupData.lookup->start(el::Host::fromStringOrThrow("9.9.9.9"_el));
    }

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableTerminal();
        lookupData.eventThread = app.createEventThread();
        lookupData.eventThread->start();
        lookupData.eventThread->events()->invoke(resolveInWorkerLoop);
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Handler runs in worker event loop: true

.. erbsland-demo-end::

Cancel a Result That Is No Longer Useful
========================================

Call ``cancel()`` when the object, request, or screen waiting for the result goes away.
Cancellation is thread-safe and idempotent, so cleanup code can call it without first checking the lookup state.

If cancellation wins before result dispatch starts, neither ``onResolved()`` nor ``onError()`` runs.
``onFinal()`` is still emitted after the lookup returns to ``Inactive``.
If a handler is already running, it finishes normally because cancellation does not interrupt user code.
Cancelling an inactive lookup or repeating cancellation has no effect.

.. erbsland-demo::
    :source: network/HostLookupCancel/main.cpp
    :exec: network/host_lookup_cancel
    :source-sha256: 466e725ab66bef56651fce78743f237579d08321c08d75499d9e29cb1c1ef049

.. code-block:: cpp

    /// Cancel a lookup when its result is no longer useful.
    /// Cancellation is safe to repeat and prevents a pending completion handler from running when it wins the race.
    void cancelUnneededLookup() {
        const auto events = el::application().events();
        lookup = events->get<el::Network>().createHostLookup();
        lookup->events()
            .onResolved([](const el::List<el::IpAddress> &) -> void {
                el::stdOut()->printLine("The cancelled lookup unexpectedly completed."_el);
            })
            .onFinal([]() -> void {
                el::stdOut()->printLine("Lookup state after final event: Inactive"_el);
                el::application().quit();
            });

        lookup->start(el::Host::fromStringOrThrow("9.9.9.9"_el));
        lookup->cancel();
    }

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableTerminal();
        app.events()->invoke(cancelUnneededLookup);
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Lookup state after final event: Inactive

.. erbsland-demo-end::

Let Ownership Define the Lookup Lifetime
========================================

Store the :cpp:type:`HostLookupPtr <erbsland::network::HostLookupPtr>` in the controller, request, or session that needs
the result.
When the last lookup pointer is released before completion, the lookup cancels the operation and suppresses its
handlers.
This lets an owning object stop work naturally when it is destroyed.

Call ``events()`` whenever you need to replace a handler.
It returns the same source-owned editor each time.
Passing an empty callback clears the corresponding handler.

It is safe to release your last lookup pointer from ``onFinal()``.
The active dispatch keeps the lookup valid until the handler returns.
Every normally completed or cancelled operation reaches this handler.

.. erbsland-demo::
    :source: network/HostLookupLifetime/main.cpp
    :exec: network/host_lookup_lifetime
    :source-sha256: 45884d77a69a603dd385d2800c97315e6924f0b2fb39ad2814331eac0f25a63c

.. code-block:: cpp

    /// Store the lookup in the object that needs its result and configure its source-owned editor directly.
    /// The lookup owns its handlers and can be released safely from its final handler.
    void resolveAndRelease() {
        const auto events = el::application().events();
        lookup = events->get<el::Network>().createHostLookup();
        lookup->events()
            .onResolved([](const el::List<el::IpAddress> &addresses) -> void {
                el::stdOut()->printLine("Resolved address: "_el, addresses[el::ItemIndex{0U}]);
            })
            .onFinal([]() -> void {
                lookup.reset();
                el::stdOut()->printLine("Lookup released from its final handler."_el);
                el::application().quit();
            });
        lookup->start(el::Host::fromStringOrThrow("9.9.9.9"_el));
    }

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableTerminal();
        app.events()->invoke(resolveAndRelease);
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Resolved address: 9.9.9.9
    Lookup released from its final handler.

.. erbsland-demo-end::
