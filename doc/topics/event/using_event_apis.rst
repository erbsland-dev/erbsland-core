..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Connecting to Sources
    single: Event Sources; Owner Loop
    single: Event Editors
    single: Event Sources; Lazy Binding

***************************
Connecting to Event Sources
***************************

An event source tells your application when an operation or resource has something to report.
You connect callbacks to those notifications and let the source's event loop call them when work becomes ready.
This page develops the source and editor concepts introduced in :doc:`events_in_erbsland_core`.
You will learn how a source acquires its loop, how to connect and change its callbacks, and how to arrange setup from
code running outside that loop.

Event Sources and Their Owner Loop
==================================

An :cpp:class:`EventSource <erbsland::event::EventSource>` represents an operation or resource that produces
notifications.
A host lookup can report resolved addresses, a network connection can report incoming data, and a subprocess can report
output or completion.
An application-part manager reports lifecycle changes as its parts start and stop.
The source defines which notifications are available and what arguments each callback receives.
Your application supplies the reactions through the source's event editor.

Once bound, a source belongs to exactly one event loop for the rest of its lifetime.
That *owner loop* dispatches all callbacks connected to the source.
An operating-system event or background worker may make a notification ready, but the application's callback runs in the
owner loop's execution context.
Connecting another callback does not select another loop or create another thread.
Callbacks on the same loop run one at a time, so a callback that blocks delays other sources sharing that loop.

.. mermaid::

    flowchart LR
        activity["Operation or resource becomes ready"] --> source["Event source"]
        source --> loop["Owner loop dispatches"]
        loop --> callback["Connected application callback"]

The source's :cpp:func:`ownerLoop() <erbsland::event::EventSource::ownerLoop>` returns the target identifying that loop.
A source that supports lazy binding can initially have no owner; its first editor access establishes one.
Retain the source's owning pointer for as long as you need its notifications.
The editor alone does not retain the source.
:doc:`callback_lifetimes` explains source and recipient ownership in more detail.

Creating and Binding Sources
============================

Direct Binding Through a Backend
--------------------------------

A backend's public interface creates sources associated with its loop.
Obtain that interface through the loop target's :cpp:func:`get() <erbsland::event::EventTarget::get>` method, then call
the appropriate factory.
For example, ``loop->get<el::Network>().createHostLookup()`` creates a host lookup owned by ``loop``.
``get<...>()`` returns a reference, so the factory is accessed with ``.``.
The template argument names the backend's public interface, such as :cpp:class:`Network <erbsland::network::Network>` or
:cpp:class:`System <erbsland::system::System>`.

Perform this setup inside a callback dispatched by the intended owner loop.
Holding its target pointer, or merely running on the same thread, does not enter that execution context.
When setup begins outside dispatch, queue it with ``loop->invoke(...)``.
Creation, callback registration, and starting the operation are separate steps: connect the reactions before starting
work that can produce notifications.

.. erbsland-in-practice-begin:: Preparing a Host Lookup

The host-lookup application keeps its source in an owning pointer and creates it in its queued setup function:

.. erbsland-demo::
    :source: network/HostLookupDomain/main.cpp
    :show: c:Keep the lookup(+0,+1), f:resolveDocumentationHost
    :sha256: b5bf997009e5f24cca8c50d7a34708e54edb8eb8daaf69eab3c3955c23ecc24f

.. code-block:: cpp

    /// Keep the lookup alive after its setup function returns.
    el::HostLookupPtr lookup;

    void resolveDocumentationHost() {
        const auto events = el::application().loop();
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

.. erbsland-demo-end::

Its entry point queues ``resolveDocumentationHost`` through ``app.loop()->invoke(...)`` before calling ``app.run()``.
The setup function therefore runs inside the main loop.
After the factory call, it connects the lookup's result and error handlers and calls ``start()``.
All lookup callbacks then run on that same main loop.

.. erbsland-in-practice-end::

Lazy Binding on the First Editor Access
---------------------------------------

Some event-capable APIs also support use without an event loop.
For example, :cpp:func:`Subprocess::create() <erbsland::system::Subprocess::create>` prepares a child process without
binding it to a loop or launching it.
You can prepare such an object in procedural code and later choose where its event callbacks should run.

For this source, the first ``events()`` call binds it to the loop currently dispatching the call.
That call must happen inside the chosen loop, and the association cannot subsequently change.
Later calls to ``events()`` must run on the same owner loop.
Lazy binding is a documented capability of particular APIs; it is not an alternative factory available for every source.
Connect before starting the operation: subprocess events that occurred before binding are not replayed.

.. erbsland-in-practice-begin:: Preparing Git Before Starting Its Event Thread

The :doc:`Git change-summary application </demo_apps/event/git_change_summary>` reports changed files by directory.
Its procedural main prepares Git, starts an event thread, and queues ``onStart()`` on that thread.
The arguments and output-capture settings are prepared before this excerpt:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.cpp
    :show: c:Preparation needs no event loop(+0,+7)
    :sha256: 7fe8bbe347938b9ccc4a57f69692a55c34e20e5bb7eea8c29cfa884899e59c25

.. code-block:: cpp

    // Preparation needs no event loop. The source has no owner yet.
    _process = el::Subprocess::create(findGit(), arguments, processOptions);
    auto loopOptions = el::EventLoopOptions{};
    loopOptions.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication);
    _worker = createEventThread(loopOptions);
    _worker->start();
    _worker->loop()->invoke([this]() -> void { onStart(); });
    _worker->join();

.. erbsland-demo-end::

The queued ``onStart()`` method makes the first ``events()`` call inside the worker loop.
This binds the source to that loop, connects its callbacks, and then launches Git:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.cpp
    :show: f:GitChangeSummaryApp::onStart
    :sha256: c8b98a943b3e48970a9f9d17085f7d844469549621a0c7c92b72d7010b2ebba4

.. code-block:: cpp

    void GitChangeSummaryApp::onStart() {
        // The first events() call binds the source permanently to this worker loop.
        _process->events()
            .onStandardOutput([this](const el::String &chunk) -> void { onStandardOutput(chunk); })
            .onExit([this](const el::ExitCode code) -> void { onExit(code); })
            .onError([this](const el::PlatformErrorContext &error) -> void { onError(error); })
            .onOutputTruncated([this]() -> void { onOutputTruncated(); })
            .onFinal([this]() -> void { onFinal(); });
        _process->start();
    }

.. erbsland-demo-end::

The application retains the source and the thread in members:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.hpp
    :show: c:Created without a loop-c:Written on the worker
    :sha256: f4d4a3ae2f200e381c89e6dcdee719d504e0b81e3854d9b6fcf2fb340ec88a72

.. code-block:: cpp

    el::SubprocessPtr _process;        ///< Created without a loop; retained through worker shutdown.
    el::ManagedEventThreadPtr _worker; ///< Runs the source's owner loop and is joined before reading results.
    GitStatusReport _report;           ///< Written on the worker; read by main only after joining.

.. erbsland-demo-end::

At ``Subprocess::create(...)``, the process has no owner loop.
Its output and completion callbacks belong to the worker loop even though main created the process object.
Main waits in ``join()`` and reads the finished report afterward; it never dispatches the application's main loop.

.. erbsland-in-practice-end::

Connecting Callbacks with an Event Editor
=========================================

An event source's ``events()`` method returns a reference to a stable, source-owned
:cpp:class:`EventEditor <erbsland::event::EventEditor>`.
Its concrete type exposes the notifications supported by that source.
An editor configures callbacks; it neither owns a thread nor starts the operation.
We recommend never storing an editor reference in a variable or member.
Keep the source's owning pointer and use ``source->events()`` directly whenever you connect or change callbacks.
This keeps the source visible at each registration and avoids mistaking the editor reference for ownership: the
reference does not keep the source alive and becomes invalid when the source is destroyed.

Access the editor and change callbacks inside the owner loop.
This keeps connection changes in the same execution context as notification delivery.
For a lazy source such as a subprocess, the first editor access selects the loop and later accesses require that same
context.
Access outside the required loop raises ``LogicFailure``.

An ``on...()`` method accepts a callback for one notification and returns the editor, allowing calls to be chained.
Registration itself does not call the callback.
As a naming convention, give application methods connected through ``on...()`` or ``add...()`` names beginning with
``on``, such as ``onStandardOutput`` or ``onStateChanged``.
This makes their event-loop execution context visible when reading the application.
The name is a recommendation, not an API requirement or a mechanism that schedules the method.

.. erbsland-in-practice-begin:: Connecting Git Output and Completion

The Git application's registration chain shown above uses ``_process->events()`` directly, without storing an editor
reference.
Its lambdas forward to named ``on...`` methods on the application, including these output and completion handlers:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.cpp
    :show: f:GitChangeSummaryApp::onStandardOutput, f:GitChangeSummaryApp::onFinal
    :sha256: 1e8a8f55a0001ae8467a1b882aeb9cc484e156b3cde4511fe96b551455b4a45c

.. code-block:: cpp

    void GitChangeSummaryApp::onStandardOutput(const el::String &chunk) {
        _report.append(chunk);
    }

    void GitChangeSummaryApp::onFinal() {
        _worker->quit();
    }

.. erbsland-demo-end::

``onStandardOutput()`` processes Git output, while ``onFinal()`` ends worker dispatch so main's ``join()`` can return.
These lambdas borrow ``this``; the application stays alive through worker shutdown and joining.
The complete walkthrough covers the failure handlers and subprocess cleanup.

.. erbsland-in-practice-end::

Replacing Handlers and Adding Subscriptions
===========================================

Replacing or Removing One Handler
---------------------------------

Each ``on...()`` method controls one callback slot, or one slot identified by that method's additional arguments.
Calling it again replaces the previous callback for that slot.
Passing an empty callback removes it.
Other handlers remain connected, and removing a handler does not itself cancel the source's operation.
Make replacements and removals inside the owner loop, just like initial registration.

.. erbsland-in-practice-begin:: Changing How Lookup Results Are Reported

Suppose a lookup already has a handler that prints every address.
During owner-loop setup, you can replace it with a compact count:

.. code-block:: cpp

    lookup->events().onResolved([](const el::List<el::IpAddress> &addresses) -> void {
        el::io::printLine("Resolved addresses: "_el, addresses.count().toSizeT());
    });

To stop reporting results, clear that slot:

.. code-block:: cpp

    lookup->events().onResolved({});

The lookup's error and final handlers remain connected.

.. erbsland-in-practice-end::

Adding Independent Observers
----------------------------

An ``add...()`` method adds an independent observer and returns an
:cpp:class:`EventSubscription <erbsland::event::EventSubscription>`.
Retain the returned handle for as long as you want that registration to remain active.
Destroying it or calling ``cancel()`` ends the registration; discarding the returned handle therefore immediately
removes the new observer.
Adding or removing one observer leaves other subscriptions active, unlike an ``on...()`` call that replaces the callback
in its slot.
A notification defines either replacement or subscription semantics; do not assume it provides both forms.

.. erbsland-in-practice-begin:: Watching a Server Part Stop

The :doc:`file-server application </demo_apps/event/file_server>` retains its lifecycle subscription around
``app.run()``:

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

Here the observer prints the server's request count during shutdown.
The local subscription lives until ``app.run()`` returns, then its destructor cancels the registration.
Another component can add its own observer without replacing this one.
The manager's control target is the application's main loop, so this observer runs there even though the server part has
its own event thread.

.. erbsland-in-practice-end::

See :ref:`Keeping a Subscription for as Long as You Need It <event-observer-lifetime>` for moving handles, cancellation,
and callback lifetime.

Using the Source's Owner Loop for Setup
=======================================

Every ``EventSource`` exposes ``ownerLoop()`` so code receiving a source can discover its execution context.
A reusable setup helper can accept the source pointer and queue its work through that target, without requiring callers
to pass the loop separately.
This also avoids accidentally pairing a source with the wrong loop argument.
Generic code receiving an ``EventEditor`` can obtain the same target through ``target()`` and retain the source through
``source()``.
These accessors help keep the source alive while arranging setup; the editor reference itself remains borrowed.

The source must already be bound before such a helper uses ``ownerLoop()``.
A lazy source initially returns a null target, so its first ``events()`` call still needs an explicitly selected loop.
After binding, queue editor access through the owner target and capture the source pointer by value to keep it alive
until setup runs.
The installed callback has its own lifetime requirements; it need not retain the source captured by the setup callback.

.. erbsland-in-practice-begin:: Attaching Lookup Diagnostics from Another Component

A diagnostics component can accept an already-bound lookup and install its result handler on the right loop.
In this small fragment, ``onLookupResolved`` is the component's existing free function with the lookup result signature:

.. code-block:: cpp

    void attachLookupDiagnostics(el::HostLookupPtr lookup) {
        lookup->ownerLoop()->invoke([lookup]() -> void {
            lookup->events().onResolved(onLookupResolved);
        });
    }

The helper returns after queuing setup.
Its caller must arrange for this registration to finish before starting work whose results it needs to observe.
For a helper that also starts the operation, register the handlers and call ``start()`` in the same queued callback.
The helper replaces the existing result handler because ``onResolved()`` has single-handler semantics.

.. erbsland-in-practice-end::

Submitting Work Directly with invoke()
======================================

:cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` queues a callback on a selected event target.
The caller supplies the callback and also decides when it becomes ready by making the invocation.
In this sense the calling application code is the producer of the work; no separate ``EventSource`` object or editor is
needed.
Calling ``invoke()`` does not bind an object to that loop.

You can invoke a target from procedural code, another event loop, or an ordinary worker thread.
The accepted callback runs later when the selected loop dispatches it, never inline inside ``invoke()``.
The caller therefore needs no event loop of its own.
Each invocation selects its target independently, whereas an event source keeps its owner loop for its lifetime.

.. erbsland-in-practice-begin:: Returning a Directory Scan to Its Workflow

The :doc:`explicit-invocation checksum packer </demo_apps/event/checksum_packer_invoke>` receives scanned files from an
ordinary worker thread and queues their delivery to its main-loop workflow:

.. erbsland-demo::
    :source: event/apps/ChecksumPackerInvoke/src/Workflow.hpp
    :show: f:start(-3,-1)
    :sha256: f245d8dc9aad3abb4841da5b90cdd3e8c5c2fdd64442a4c98ed69952a162c50e

.. code-block:: cpp

    auto events = el::currentLoop();
    auto workflow = shared_from_this();
    _scanner.scan([events, workflow](FileWithDigestList fileList, std::exception_ptr error) -> void {
        events->invoke([workflow, fileList = std::move(fileList), error = std::move(error)]() mutable -> void {
            workflow->onScanned(std::move(fileList), std::move(error));
        });
    });

.. erbsland-demo-end::

``start()`` runs on the main loop and saves that target before launching the scanner.
When scanning finishes, the outer callback runs on the scan worker and triggers the invocation.
The inner callback owns the transferred results and calls ``onScanned()`` on the saved loop.

.. erbsland-in-practice-end::

Introducing bindEvent()
=======================

:cpp:func:`bindEvent() <erbsland::event::bindEvent>` packages a callback and its receiving loop into a callable.
When a producer calls that callable, it queues delivery on the captured target.
Like ``invoke()``, it never calls the receiving handler synchronously.
The overload without an explicit target captures the current loop when the binding is created, so create it inside the
intended receiving loop.
An explicit target overload is available when that loop is already known.

A binding is useful when an API accepts an ordinary callback but does not know which loop should receive its result.
Connecting a callback directly to a source's editor already gives it the source's owner-loop context; it needs no
additional binding to run there.
Binding a callback packages a delivery route; it does not rebind an event source.

.. erbsland-in-practice-begin:: Expressing the Scan Result as a Binding

The :doc:`checksum packer with bindings </demo_apps/event/checksum_packer>` expresses the preceding return path with one
connection:

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Workflow.hpp
    :show: f:start(-3,-1)
    :sha256: 80e107731789a49de93896b0dde89c0d1f1b33264c64e3f53a753354dbe76534

.. code-block:: cpp

    _scanner.scan(el::bindEvent(shared_from_this(), &Workflow::onScanned));

.. erbsland-demo-end::

The scanner calls the returned callable when its results are ready.
The binding queues ``Workflow::onScanned()`` on the loop captured in ``start()`` and retains the shared workflow for
delivery.

.. erbsland-in-practice-end::

Continue with :doc:`event_bindings` for target selection, argument ownership, and weak recipient bindings.
:doc:`handling_event_errors` explains callback exceptions and the owner loop's failure policy.
