..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Connecting to Sources
    single: Event Sources; Handlers
    single: Event Editors

***************************
Connecting to Event Sources
***************************

A host lookup takes time to produce a result.
Your application can start the lookup and continue handling other work while it waits.
You register callbacks to say what should happen when the lookup succeeds or fails.

This page shows how to connect those callbacks, where they run, and what keeps the lookup alive.
It assumes you have chosen an application structure as described in :doc:`using_event_loops`.
The same setup pattern applies to other event sources, although each source defines its own notifications.

Creating a Source and Registering Handlers
==========================================

The application below prints the addresses of ``core.erbsland.dev`` and then exits.
Its setup function does three things: creates the lookup, connects its callbacks, and starts the operation.

That setup must run on the loop that will own the lookup.
In ``main()``, ``app.events()->invoke(resolveDocumentationHost)`` queues the setup function for the main event loop.
``app.run()`` runs the loop, which calls the queued function and later delivers the lookup result.
Queuing setup is necessary even though ``main()`` runs on the same thread: the source requires execution inside its
owning loop.

The ``lookup`` variable is stored outside the setup function so it survives that function's return.
The example omits includes and the namespace wrapper; the stored pointer, setup function, and application entry point
are shown together.

.. erbsland-demo::
    :source: network/HostLookupDomain/main.cpp
    :show: c:Keep the lookup(+0,+1), f:resolveDocumentationHost(+0,+7)
    :exec: network/host_lookup_domain
    :show-cmd-line:
    :sha256: ac766f144867e2239d35915a0f2f38cf2717ac958475b8eaf83cb2e378f2a07c

.. code-block:: cpp

    /// Keep the lookup alive after its setup function returns.
    el::HostLookupPtr lookup;

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

.. erbsland-demo-run::

    .. code-block:: console

        $ network/host_lookup_domain

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mcore.erbsland.dev resolved to:
          2a06:98c1:3121::c
          2a06:98c1:3120::c
          188.114.96.12
          188.114.97.12␛[0m

.. erbsland-demo-end::

The output is from an example run; DNS addresses can differ between runs.

Inside the setup function, ``events->get<el::Network>()`` accesses the loop's network service.
``createHostLookup()`` then creates a lookup belonging to that loop.
You do not need to create a network backend yourself.

``lookup->events()`` lets you configure the lookup's callbacks.
Here, ``onResolved()`` installs the function that prints the addresses, and ``onError()`` installs the function that
reports a lookup failure.
``onFinal()`` installs the function that asks the application to quit after normal completion.
Each registration returns the same editor, so the calls can be chained.

Registration does not start the lookup or call these functions.
The last statement, ``start()``, begins the operation after all three handlers are in place.

Callback Execution Order
========================

The setup function returns while the lookup is still pending.
When the result becomes available, the same event loop calls the registered handler.
During setup, your code registers the handlers before starting the operation:

.. mermaid::

    sequenceDiagram
        participant L as Application code
        participant S as Lookup
        L->>S: Create and register handlers
        L->>S: start()
        S-->>L: Return while pending
        Note over L: Setup callback returns

Later, a successful result causes the lookup to call the result handler and then the final handler.
The final handler in this example requests application shutdown:

.. mermaid::

    sequenceDiagram
        participant L as Application code
        participant S as Lookup
        S->>L: onResolved(addresses)
        L-->>S: Result handler returns
        S->>L: onFinal()
        L-->>S: Return after requesting quit
        Note over L,S: The main loop can now exit

The participants show application code and the lookup as separate roles, not separate threads.
Both setup and the lookup's handlers execute on the main loop's thread.

For a lookup failure, ``onError()`` runs instead of ``onResolved()``.
In this example it prints the error and requests a failure exit code.
The final callback also runs after a normally returning error handler.

The lookup callbacks run on the main loop because that is where this lookup was created.
They do not run on an arbitrary background thread.
Keep them short so the loop can continue processing other work.

The address list is borrowed for the duration of the result callback.
Printing it there is safe; if your application needs the addresses later, copy the list into state that survives the
callback.

Source and Handler Lifetime
===========================

The stored ``HostLookupPtr`` owns the lookup.
If it were only a local variable inside ``resolveDocumentationHost()``, releasing that variable on return would destroy
the lookup and cancel its pending work.
In a larger application, store this pointer in the component that needs the result.

The lookup owns its registered callbacks.
You do not need a separate subscription handle for ``onResolved()``, ``onError()``, or ``onFinal()``.
The object returned by ``lookup->events()`` is an *event editor*: it changes those registrations, but a reference to it
does not keep the lookup alive.
Keep the lookup pointer when you need to change its handlers later.

These ownership rules are enough for this example, whose callbacks do not capture another object.
:doc:`callback_lifetimes` explains how to keep callback recipients alive and how to release or cancel an operation.

Event Targets and Event Editors
===============================

The example uses the same method name for two different jobs:

.. list-table::
    :header-rows: 1
    :widths: 45 55

    * - Expression
      - Purpose
    * - ``app.events()->invoke(...)``
      - Queue work on the application's event loop.
    * - ``lookup->events().onResolved(...)``
      - Configure what the lookup does when it has a result.

The application returns an :cpp:class:`Events <erbsland::event::Events>` pointer, also called a *target*.
The lookup returns an :cpp:class:`EventEditor <erbsland::event::EventEditor>` reference.
That difference in return types accounts for ``->`` in one expression and ``.`` in the other.
The lookup itself is an :cpp:class:`EventSource <erbsland::event::EventSource>`: an object that produces notifications.

Replacing and Clearing Handlers
===============================

Each ``on...()`` method stores one callback.
Calling ``onResolved()`` again replaces the previous result callback; it does not add a second listener.
The other handlers remain in place.
For example, this fragment could replace the printing callback during setup, before ``start()``:

.. code-block:: cpp

    lookup->events().onResolved([](const el::List<el::IpAddress> &addresses) -> void {
        el::stdOut()->printLine("Resolved address count: "_el, addresses.count().toSizeT());
    });

An empty callback clears that handler:

.. code-block:: cpp

    lookup->events().onResolved({});

Clearing the result handler does not cancel the lookup or remove its error and final handlers.
Make handler changes on the lookup's owning loop, just like the initial setup.
If another thread needs to request a change, queue it through the lookup's ``ownerEvents()`` target and capture the
lookup pointer by value to keep it alive until that queued change runs.

Some APIs support independent listeners through ``add...()`` methods instead.
Those methods return a subscription handle that you must retain for as long as you want to listen.
The :ref:`observer example <event-observer-lifetime>` explains this separate registration model.

Operation Errors and Callback Exceptions
========================================

``onError()`` handles failures discovered while resolving the host.
An invalid call to ``start()`` can instead throw immediately, and an exception thrown by a callback follows the event
loop's error policy.
In particular, a throwing result or error callback skips the lookup's ``onFinal()`` callback.
Use object ownership and destructors for cleanup that must always happen.
:doc:`handling_event_errors` explains how to choose recovery or termination and observe captured callback exceptions.

The :doc:`host lookup topic </topics/network/resolving-hosts-asynchronously>` covers lookup-specific completion and
cancellation behavior.
Other event sources define their own timing and error contracts; register their handlers before starting work and
consult their documentation for the notifications they provide.


