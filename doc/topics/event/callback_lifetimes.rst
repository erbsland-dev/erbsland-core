..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Callback Lifetime
    single: Callbacks; Ownership
    single: Event Subscriptions; Cancellation

****************************************
Keeping Callbacks and Their Owners Alive
****************************************

A callback may run after the function that registered it has returned.
Its recipient may also disappear while an operation is still pending.
You need an ownership design that keeps useful work alive and prevents obsolete work from accessing destroyed objects.

This page builds on :doc:`using_event_apis`.
It explains value and pointer captures, weak recipients, subscription handles, and source-specific cleanup.
The examples separate the lifetime of an operation from the lifetime of the object waiting for its result.

Source and Callback Ownership
=============================

A source owns its handlers, and its editor only borrows access to them.
A controller usually retains the source because it needs that operation's result.
The source in turn retains its owner target, which identifies the loop that dispatches its callbacks.
A timer is another retained object: keeping the timer handle alive keeps its scheduled activity available until it is
stopped or destroyed.

For a source-owned handler, the owning relationships and borrowed editor access are:

.. mermaid::

    flowchart TD
        C["Controller"] -->|Shared pointer| S["Source"]
        S -->|Shared pointer| T["Owner target"]
        S -->|Owns| H["Handlers"]
        S -->|Owns| E["Editor"]
        C -.->|Borrows| E

A handler's captures determine what it retains beyond this graph.
A value capture belongs to the callback, a shared pointer retains its recipient, and a borrowed pointer retains nothing.
Timers similarly own their callbacks, while a separate timer handle keeps the timer alive.
Subscription ownership is explained below because it controls a registration independently of the publisher's lifetime.

The target still needs a running loop to execute callbacks.
Keeping a pointer does not start dispatch or keep a worker running indefinitely.
Likewise, retaining a source and retaining an observer subscription solve different problems: the source retains the
operation, while the subscription controls one registration with a publisher.

The lookup lifetime demo stores the source until its final callback and then releases it.

.. erbsland-demo::
    :source: network/HostLookupLifetime/main.cpp
    :exec: network/host_lookup_lifetime
    :sha256: 8cbd4fcb85b2a3b6063591bd51c43d8021035573a42ff79813f993ec7b829854

.. code-block:: cpp

    /// Store the lookup in the object that needs its result and configure its source-owned editor directly.
    /// The lookup owns its handlers and can be released safely from its final handler.
    void resolveAndRelease() {
        const auto events = el::application().events();
        lookup = events->get<el::Network>().createHostLookup();
        lookup->events()
            .onResolved([](const el::List<el::IpAddress> &addresses) -> void {
                el::stdOut()->printLine("Resolved address: "_el, addresses.first());
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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mResolved address: 9.9.9.9
        Lookup released from its final handler.␛[0m

.. erbsland-demo-end::

Here, the callback does not capture the lookup pointer.
The outer retained pointer keeps the source alive, and the final handler explicitly releases that pointer.
If the last owning pointer were released before completion, this lookup would cancel its operation and suppress its
handlers.
That is the lookup's lifetime contract; check other sources rather than assuming identical destruction behavior.

Value Captures and Borrowed References
======================================

Capturing an input by value gives queued work its own copy.
Later changes to the original value do not alter that copy.
A reference capture instead depends on the original object staying alive until every use of that reference finishes.
Local references are therefore safe only when the surrounding scope deliberately outlives the callbacks.

The following example contrasts a copied caption with a deliberately retained object.
Both callbacks execute after their setup code, and the strong pointer capture keeps its object alive even after the
original pointer is reset.
The demo pumps a local loop before returning, so all work has a bounded lifetime.

.. erbsland-demo::
    :source: event/EventSource/WeakController.cpp
    :show: f:retainingValues
    :sha256: 5eeff80efb5e52a810402875379201f3ce0806dc185cba8d2dadba561259481a

.. code-block:: cpp

    void retainingValues() {
        const auto loop = el::EventLoop::create();
        auto caption = "Skovens lys"_el;
        loop->invoke([caption]() -> void { el::io::printLine("Captured value: "_el, caption); });
        caption = "Havets farver"_el; // This change cannot alter the captured value.

        auto retainedCaption = std::make_shared<el::String>(caption);
        loop->invoke([retainedCaption]() -> void { el::io::printLine("Retained object: "_el, *retainedCaption); });
        retainedCaption.reset(); // The queued callback still owns the object.
        (void)loop->runUntilIdle();
    }

.. erbsland-demo-end::

A ``[this]`` capture has the same lifetime obligation as any other borrowed pointer.
It does not keep the object alive.
It can be appropriate when an enclosing owner guarantees that callbacks have stopped before destruction, but the syntax
alone supplies no such guarantee.
For a result that must finish even after its initiating scope ends, a strong ``shared_ptr`` capture can express that
intent more directly.

Shared ownership protects object lifetime, not mutable data.
Two loops that capture the same shared object still need synchronization or a design that confines its state to one
loop.
Similarly, callback arguments received by reference remain borrowed: copy needed results, as shown in
:doc:`using_event_apis`, rather than saving a pointer to them.

Weak Captures for Optional Recipients
=====================================

Often the operation is useful only while its controller still exists.
A weak capture expresses that relationship without forcing the controller to survive until every callback has run.
At callback entry, ``lock()`` either gives a temporary strong pointer for the duration of the work or reports that the
recipient has gone away.

This controller owns a lookup and configures its result handler after the controller has been placed in shared
ownership.
The handler captures ``weak_from_this()`` and accesses the controller only after successfully locking it.
Calling ``weak_from_this()`` in the constructor would not establish that relationship because shared ownership is not
yet in place there.

.. erbsland-demo::
    :source: event/EventSource/IllustrationController.hpp
    :sha256: 61ac34fca320bc4e387f497ac17dbb985d14fc4c047572a8775b6a40f6bd9a5e

.. code-block:: cpp

    /// A recipient that owns its lookup without being retained by the lookup's callbacks.
    /// @notest{Compiled documentation demo.}
    class IllustrationController final : public std::enable_shared_from_this<IllustrationController> {
    public:
        /// Create a controller on the target's owner loop.
        /// @param target The target for lookup callbacks.
        /// @param caption The illustration caption printed with the result.
        IllustrationController(const el::EventsPtr &target, el::String caption) :
            _lookup{target->get<el::Network>().createHostLookup()}, _caption{std::move(caption)} {}

    public: // accessors
        /// Access the owned lookup for deliberate independent retention.
        [[nodiscard]] auto lookup() const -> el::HostLookupPtr { return _lookup; }

    public: // operations
        /// Start a lookup with a weak reference to this recipient.
        /// Must be called on the owner event loop while the lookup is inactive.
        /// @param host The numeric host used by the demonstration.
        void start(el::Host host) {
            _lookup->events().onResolved([weak = weak_from_this()](const el::List<el::IpAddress> &addresses) -> void {
                if (const auto self = weak.lock(); self != nullptr) {
                    el::io::printLine(self->_caption, ": "_el, addresses.first());
                } else {
                    el::io::printLine("Recipient was released; result ignored."_el);
                }
            });
            _lookup->start(std::move(host));
        }

    private:
        el::HostLookupPtr _lookup; ///< Source owned by the recipient.
        el::String _caption;       ///< State accessed only on the lookup's owner loop.
    };

.. erbsland-demo-end::

A strong self-capture here would create a cycle: the controller owns the lookup, the lookup owns the handler, and the
handler would own the controller.
A timer owned by a controller can form the same cycle through its callback.
An explicit teardown can break such cycles, but weak captures avoid them when continued recipient ownership is not the
purpose of the operation.

The consumer below keeps one controller alive and releases another before dispatching its result.
It deliberately retains the second source independently so you can see its weak handler ignore the vanished recipient.
Normally, releasing that controller's last source pointer would cancel its lookup instead.

.. erbsland-demo::
    :source: event/EventSource/WeakController.cpp
    :show: f:weakController
    :exec: event/event_source --demo WeakController
    :sha256: 70fd9fc20279e6bf4e803aeee4cf96e0148b947ef52dcaa485c29175589eed43

.. code-block:: cpp

    void weakController() {
        retainingValues();
        const auto loop = el::EventLoop::create();
        auto controller = std::shared_ptr<IllustrationController>{};
        auto abandonedSource = el::HostLookupPtr{};
        loop->invoke([&controller, &abandonedSource]() -> void {
            const auto target = el::currentEvents();
            controller = std::make_shared<IllustrationController>(target, "Skovens lys"_el);
            controller->start(el::Host::fromStringOrThrow("127.0.0.1"_el));

            auto temporary = std::make_shared<IllustrationController>(target, "Havets farver"_el);
            abandonedSource = temporary->lookup();
            temporary->start(el::Host::fromStringOrThrow("127.0.0.2"_el));
            temporary.reset(); // The source survives, but its weak callback cannot access the recipient.
        });
        (void)loop->runUntilIdle();
        if (const auto error = loop->takeError(); error != nullptr) {
            std::rethrow_exception(error);
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mCaptured value: Skovens lys
        Retained object: Havets farver
        Skovens lys: 127.0.0.1
        Recipient was released; result ignored.␛[0m

.. erbsland-demo-end::

The first callback can access its controller, while the second observes an expired weak pointer.
Locking a weak pointer does not make old requests current again.
If a surviving controller can start a newer operation, it may also need request identity or state checks to reject
obsolete results.

.. _event-observer-lifetime:

Subscription Ownership
======================

An :cpp:class:`EventSubscription <erbsland::event::EventSubscription>` is move-only.
Moving it transfers responsibility for cancellation; the moved-from handle becomes inactive.
Move assignment first cancels any registration already owned by the destination.
Destroying a handle cancels its registration, and ``cancel()`` lets you end observation before the handle's scope ends.
Repeated cancellation is harmless.

A subscription retains a registration containing the observer callback.
It does not keep the publisher alive: publisher destruction makes the registration inactive even if the handle remains.
This differs from retaining an event source to keep an operation alive.

.. mermaid::

    flowchart TD
        O["Observer object"] -->|Owns| H["Subscription handle"]
        H -->|Retains| R["Registration and callback"]
        R -.->|Checks lifetime; does not retain| P["Publisher"]

The dotted edge expresses a lifetime dependency, not ownership.
A small object can store the subscription so observation ends when that object is destroyed:

.. erbsland-demo::
    :source: event/EventSource/LifecycleObserver.hpp
    :sha256: 85a5aa56857bd640a4a8a085705ce07c1c919c1f0c1ad80b7619b1b125910a51

.. code-block:: cpp

    /// Own one lifecycle registration independently of the publisher.
    /// @notest{Compiled documentation demo.}
    class LifecycleObserver final {
    public:
        /// Transfer a registration into this observer.
        /// @param subscription The registration to retain.
        explicit LifecycleObserver(el::EventSubscription subscription) : _subscription{std::move(subscription)} {}

    public: // accessors
        /// Test whether the retained registration remains active.
        [[nodiscard]] auto isActive() const noexcept -> bool { return _subscription.isActive(); }

    public: // operations
        /// Stop observing future notifications; safe to repeat.
        void stopWatching() noexcept { _subscription.cancel(); }

    private:
        el::EventSubscription _subscription; ///< Cancels automatically when the observer is destroyed.
    };

.. erbsland-demo-end::

This demo gives a detached part manager a local control loop, adds two independent observers, and moves one subscription
into that object.
It first dispatches the preparation notification, then cancels one observer before starting the manager.
The diagnostic observer remains active and continues to receive later transitions.

.. erbsland-demo::
    :source: event/EventSource/ObserverLifetime.cpp
    :exec: event/event_source --demo ObserverLifetime
    :sha256: e7259dd27afdae5c65d7ae13bf359cb92253121826f1d55962869df269e76815

.. code-block:: cpp

    /// Retain independent observers, transfer a subscription, and cancel before the next notification.
    /// Cancelling a subscription changes observation, not the publisher's lifecycle.
    void observerLifetime() {
        const auto loop = el::EventLoop::create();
        auto manager = el::ApplicationPartManager::create(loop);
        auto observedCount = 0;
        auto diagnostic = manager->events().addStateChanged([](const el::ApplicationPartManagerState) -> void {
            el::io::printLine("Diagnostic observer received a transition."_el);
        });
        auto subscription = manager->events().addStateChanged(
            [&observedCount](const el::ApplicationPartManagerState) -> void { ++observedCount; });
        auto observer = LifecycleObserver{std::move(subscription)};
        el::io::printLine("Moved-from handle active: "_el, subscription.isActive());

        // Discarding a returned handle cancels this registration at the end of the expression.
        (void)manager->events().addStateChanged([](const el::ApplicationPartManagerState) -> void {
            el::io::printLine("This discarded observer is never called."_el);
        });
        manager->prepare();
        (void)loop->runUntilIdle();
        el::io::printLine("Observed preparation transitions: "_el, observedCount);

        observer.stopWatching();
        observer.stopWatching();
        manager->start(); // With no parts, startup still produces manager lifecycle transitions.
        (void)loop->runUntilIdle();
        el::io::printLine("Count after cancellation: "_el, observedCount);
        el::io::printLine("Cancelled handle active: "_el, observer.isActive());

        manager.reset();
        el::io::printLine("Handle active after publisher destruction: "_el, diagnostic.isActive());
        if (const auto error = loop->takeError(); error != nullptr) {
            std::rethrow_exception(error);
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMoved-from handle active: false
        Diagnostic observer received a transition.
        Observed preparation transitions: 1
        Diagnostic observer received a transition.
        Diagnostic observer received a transition.
        Count after cancellation: 1
        Cancelled handle active: false
        Handle active after publisher destruction: false␛[0m

.. erbsland-demo-end::

The discarded registration never receives a notification because its temporary handle is destroyed immediately.
Destroying the publisher invalidates the diagnostic handle even though that handle still exists.
``isActive()`` reports whether a registration remains active; it does not wait for callback completion.
The observer callback in this demo borrows the local counter safely because all notifications are dispatched before the
scope exits.
A long-lived observer should instead use owned state or a lifetime-aware recipient capture.

Cancellation and Running Callbacks
----------------------------------

Cancellation makes the registration inactive for later notifications, but it does not wait for dispatch already in
progress.
A callback already being dispatched can still execute or finish after cancellation returns.
It is not a join barrier.
This sequence shows cancellation from another thread while a callback is already running:

.. mermaid::

    sequenceDiagram
        participant D as Dispatch thread
        participant C as Cancelling thread
        participant S as Subscription
        activate D
        Note over D: Callback is running
        C->>+S: cancel()
        S-->>-C: Returns
        Note over C: Caller continues
        Note over D: Callback finishes later
        deactivate D

The dispatch thread's activation spans the cancellation call and its return.
There is no wait for that callback in ``cancel()``.

The cancelling thread can continue before the running callback has returned.
If another thread can be running a callback that borrows your object, cancelling the subscription alone does not prove
that destroying the object is safe.
A weak capture with a temporary strong pointer, or explicit synchronization with the dispatching component, addresses
that separate lifetime problem.

Cancellation Scope
==================

Several actions can make future callbacks disappear, but their scopes differ:

.. list-table::
    :header-rows: 1
    :widths: 25 40 35

    * - Action
      - What it controls
      - What it does not establish
    * - Subscription ``cancel()``
      - One observer registration
      - Operation completion or a finished running callback
    * - ``onResolved({})``
      - One source-owned result handler
      - Cancellation of the lookup or its other handlers
    * - Timer ``stop()``
      - Future occurrences of that timer
      - Interruption of an executing timer callback
    * - Lookup ``cancel()``
      - The lookup's pending operation
      - Cancellation of unrelated work on the loop
    * - Releasing a source
      - Your ownership of the source
      - Destruction while other owners still retain it

The handler example in :doc:`using_event_apis` explains clearing a handler without cancelling the operation.
The observer demo above shows cancellation while the manager continues its lifecycle.
For a timer, retain the :cpp:class:`EventTimer <erbsland::event::EventTimer>` handle when you need to stop scheduled
activity; its callback cannot be interrupted once executing.
:doc:`scheduling_work` demonstrates timer restart, stop, and destruction with compiled examples.

Lookup cancellation is operation-specific.
This example starts a numeric lookup and cancels it before the queued result can begin dispatching:

.. erbsland-demo::
    :source: network/HostLookupCancel/main.cpp
    :exec: network/host_lookup_cancel
    :sha256: 518bf37ecb077805e2536ae5a85780894bfe02247013400bd5f169aecdb67971

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLookup state after final event: Inactive␛[0m

.. erbsland-demo-end::

When this cancellation wins, the result and error callbacks are suppressed and the retained lookup returns to its
inactive state before ``onFinal()`` runs.
If a result callback already started, cancellation cannot interrupt it.
Releasing the source as well can suppress the final notification, so a cancelled operation and an observed final
callback are separate decisions.
Full shutdown also needs to coordinate producers, loops, and thread joins; handle cancellation alone does not accomplish
that sequence.
:doc:`shutting_down_event_work` follows that sequence from resource closure to completed thread execution.

Event Editor Lifetime
=====================

Generic code can use :cpp:func:`EventEditor::source() <erbsland::event::EventEditor::source>` to obtain an owning source
pointer and :cpp:func:`EventEditor::target() <erbsland::event::EventEditor::target>` to retain its callback target.
Retain the source deliberately if generic code must keep it alive; retaining only the target leaves the source's
lifetime unchanged.

These accessors do not turn the editor reference into an owning object.
Once the source dies, the borrowed editor is invalid and cannot be used even to call ``source()``.
Store a source pointer when you need later edits, then obtain its editor again on the owner loop.

Releasing Sources from Callbacks
================================

The opening lookup demo releases its last external source pointer from ``onFinal()``.
This is safe because the lookup keeps itself alive during active dispatch and copies the selected handler before calling
user code.
After the handler returns, dispatch can release the source and destroy its editor.
Any previously borrowed editor must then be treated as invalid.

Whether other sources permit releasing the source, replacing the current handler, or destroying the recipient from a
callback depends on their reentrancy and dispatch-lifetime contracts.
A weak recipient capture also keeps a successfully locked recipient alive until the callback returns, so resetting
another owning pointer during that callback does not immediately destroy it.

Finally, cleanup needs an error path as well as a success path.
A lookup's throwing result or error handler restores the inactive state but skips its final notification.
Use ordinary ownership and destructors for resources that must always be released, and use final callbacks for the
source-specific sequencing they actually promise.
:doc:`/topics/network/resolving-hosts-asynchronously` explains lookup cancellation and completion in detail.

:doc:`event_workflows` combines these lifetime rules with operation state to connect several asynchronous steps into
one task.
