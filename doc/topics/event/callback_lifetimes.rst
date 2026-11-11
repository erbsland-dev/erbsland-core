..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Callback Lifetime
    single: Callbacks; Lambda Captures
    single: Callbacks; Ownership Cycles
    single: Event Subscriptions; Cancellation
    single: Event Editors; Lifetime

********************************
Callback Lifetimes and Ownership
********************************

You start an operation, connect a callback, and return to the event loop.
The code compiles, but the callback never runs.
Another version works until a controller is destroyed, then crashes.
A third appears to work perfectly while retaining objects that should have been released.
These problems often have the same cause: the objects needed by a callback have different lifetimes.

This page explains how to keep an operation available, give a callback safe access to its data, and release both when
work ends.
You will learn what C++ lambdas capture, when to use strong or weak pointers, and how subscriptions and event editors
fit into that ownership design.
It builds on :doc:`using_event_apis`; you do not need to be familiar with lambda capture initializers.

Why a Callback Never Arrives
============================

Registering a callback prepares work for later.
The function that registers it can return before the event loop calls it, even when an operation can produce its result
immediately.
The source must survive that interval.

An event source owns its registered handlers.
Connecting a handler does not create an outside owner for the source, and starting an operation is not a promise that
the source will keep itself alive until completion.
If a local ``shared_ptr`` is the last owning pointer, leaving its scope destroys the source.
For a host lookup, destruction cancels the operation and suppresses notifications that have not started dispatching.
There is then no source left to deliver the result you were waiting for.

.. erbsland-in-practice-begin:: A Server Lookup That Loses Its Result

Suppose a command-line tool resolves a configured server name or address before a client uses it.
The :doc:`/demo_apps/event/lookup_lost_source` application queues ``begin()`` on its main loop after parsing the
options.
It accepts a host and an application deadline, defaulting to ``127.0.0.1`` and one second.
A numeric address uses the lookup's asynchronous result delivery without depending on an external DNS service.

The setup looks reasonable: create a lookup, register its handlers, then start it.
The mistake is the local ``lookup`` pointer:

.. erbsland-demo::
    :source: event/apps/LookupLostSource/LookupLostSourceApp.hpp
    :show: f:begin, f:onTimeout, c:Server selected(+0,+3)
    :exec: event/lookup_lost_source
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 1756f42aa20efc653ad287b8fa5f87b6a359133df4498c54152a8f562b9469a0

.. code-block:: cpp

    void begin() {
        el::io::printLine("Looking up server: "_el, _host.toString());
        // This deadline belongs to the application, independently of the lookup.
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        const auto lookup = loop()->get<el::Network>().createHostLookup();
        lookup->events()
            .onResolved([host = _host](const el::List<el::IpAddress> &addresses) -> void {
                for (const auto &address : addresses) {
                    el::io::printLine("Server address for "_el, host.toString(), ": "_el, address);
                }
            })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                quit(_exitCode);
            });
        lookup->start(_host);
        el::io::printLine("Setup complete; waiting for the result."_el);
        if (_timeout.isZero()) {
            onTimeout();
        }
        // BUG: the only owning pointer is destroyed here, before result dispatch.
    }

    void onTimeout() {
        if (_finished) {
            return;
        }
        _finished = true;
        el::io::printLine("Deadline reached without a lookup result."_el);
        quit(el::ExitCode::failure());
    }

    el::Host _host;                                  ///< Server selected on the command line.
    el::TimeDelta _timeout;                          ///< Independent application deadline.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Result reported when the operation finishes.
    bool _finished{false};                           ///< Prevents a late deadline from changing a completed result.

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_lost_source

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLooking up server: 127.0.0.1
        Setup complete; waiting for the result.
        Deadline reached without a lookup result.␛[0m

.. erbsland-demo-end::

There is no ``Server address for ...`` line.
``begin()`` destroys its only source pointer before the loop can deliver the queued address.
Neither the result handler nor the final handler runs.
The independent application deadline reports the missing result and exits with failure; it is not a lookup notification.
Making that deadline longer would not repair the missing owner.

.. erbsland-in-practice-end::

Keeping Sources in an Owning Object
-----------------------------------

A useful default is to store each source in the application or controller responsible for its operation.
That owner also stores the state used by its callbacks.
For several independent operations, an application can keep a collection of operation controllers and remove each one
when its work finishes.
The collection supplies the outside ownership; a controller that is itself only a temporary local object does not solve
the problem.

A local source pointer is also appropriate when its scope deliberately encloses the whole period of event processing.
The distinction is whether the pointer survives until the operation no longer needs it, rather than whether the pointer
is a member or a local variable.

.. erbsland-in-practice-begin:: Retaining the Lookup Until Completion

The :doc:`/demo_apps/event/lookup_owned_source` version stores the lookup in ``_lookup``.
Its application object encloses ``app.run()``, so the application and its members remain available throughout dispatch:

.. erbsland-demo::
    :source: event/apps/LookupOwnedSource/main.cpp
    :show: f:main
    :sha256: 8dd886fa6eabee30c80fc51dbe24fe20622429f1eaab9ce7502d15599511adb4

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = demo::LookupOwnedSourceApp{argc, argv};
        return app.run();
    }

.. erbsland-demo-end::

The setup still runs on the owner loop.
The result handler owns a copy of the requested host, and the final handler releases the member after the lookup has
finished:

.. erbsland-demo::
    :source: event/apps/LookupOwnedSource/LookupOwnedSourceApp.hpp
    :show: f:begin, f:cleanup, c:Keeps the source alive(+0,+4)
    :exec: event/lookup_owned_source
    :show-cmd-line:
    :sha256: 4c4a72f80fa191fd4b129a02cc543a01ae82899997669f714e81c8d4676cbd10

.. code-block:: cpp

    void begin() {
        el::io::printLine("Looking up server: "_el, _host.toString());
        // This deadline belongs to the application, independently of the lookup.
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        _lookup = loop()->get<el::Network>().createHostLookup();
        _lookup->events()
            .onResolved([host = _host](const el::List<el::IpAddress> &addresses) -> void {
                for (const auto &address : addresses) {
                    el::io::printLine("Server address for "_el, host.toString(), ": "_el, address);
                }
            })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                _lookup.reset();
                quit(_exitCode);
            });
        _lookup->start(_host);
        el::io::printLine("Setup complete; waiting for the result."_el);
        if (_timeout.isZero()) {
            onTimeout();
        }
        // Otherwise, the member retains the lookup after this function returns.
    }

    void cleanup() noexcept override { _lookup.reset(); }

    el::HostLookupPtr _lookup;                       ///< Keeps the source alive between setup and completion.
    el::Host _host;                                  ///< Server selected on the command line.
    el::TimeDelta _timeout;                          ///< Independent application deadline.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Result reported when the operation finishes.
    bool _finished{false};                           ///< Prevents a late deadline from changing a completed result.

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_owned_source

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLooking up server: 127.0.0.1
        Setup complete; waiting for the result.
        Server address for 127.0.0.1: 127.0.0.1␛[0m

.. erbsland-demo-end::

Now the same request prints its address.
The member remains alive after setup returns, and this lookup permits releasing it from its final handler.
The application deadline also releases it if no result arrives.
``cleanup()`` provides unconditional release when the application finishes, including when an exception ends dispatch.
The deadline's and final handler's ``[this]`` captures borrow an application that remains alive throughout ``run()``.

.. erbsland-in-practice-end::

Releasing one shared pointer does not destroy an object if another owning pointer still exists.
Think in terms of the last owner, including owners hidden in callback captures.
Also distinguish object lifetime from execution: a retained target does not keep its event loop running.
The application must continue dispatching until required work finishes.

Cleanup must cover failure as well as success.
A final callback is useful for the sequencing promised by a particular source, but it is not a universal destructor.
For example, a throwing host-lookup result or error handler skips its final notification.
Use ordinary ownership and the application's cleanup path for unconditional release.
The source-specific completion rules are explained in :doc:`/topics/network/resolving-hosts-asynchronously`.

What a Lambda Actually Keeps
============================

A lambda creates a callable object.
Its capture list describes the state stored inside that object, and the event system stores that callable for later use.
A capture is initialized when the lambda is created, not each time the callback runs.
This is why a callback can retain some inputs even after the registering function returns.

With ``[value]``, the callable stores a copy of the named variable.
With ``[&value]``, it borrows the original variable, which must remain alive for every later access.
A callback that captures a local variable by reference and runs after that variable's scope has ended has a dangling
reference.
Copy small inputs or give the callback an owning object instead.

The type being copied matters:

.. list-table::
    :header-rows: 1
    :widths: 28 72

    * - Captured by value
      - What remains available
    * - An owning value such as ``String``
      - The callback's own value, independent of later assignment to the original variable.
    * - A ``shared_ptr``
      - Another owning pointer to the same object. The object remains alive while that owner exists.
    * - A ``weak_ptr``
      - A way to check for the object later, without preventing its destruction.
    * - A raw pointer, view, or reference wrapper
      - A copied handle to borrowed storage. Its underlying object still needs another owner.

Copying a shared pointer does not copy the pointed-to object.
The callback and other owners see the same object, including changes to its state.
Likewise, owning that object does not make concurrent access to it safe.
Keep mutable state on one loop or coordinate access as described in :doc:`communicating_between_loops`.

Capture Initializers: Choosing What to Store
--------------------------------------------

The form ``[name = expression]`` creates a named capture initialized from an expression.
Despite the ``=`` sign, it is initialization of new callback state, not assignment to an existing outside variable.
The name is available inside the lambda.
This lets you copy a member into a value, move an owned value, or construct a different pointer type explicitly.

.. erbsland-in-practice-begin:: Capturing the Requested Server

In the retained lookup, ``[host = _host]`` copies the configured host into the callback.
Inside that lambda, ``host`` names the copy; accessing it does not require a pointer to the application.
A later assignment to ``_host`` would not change which host that callback reports.

The same syntax expresses other ownership choices:

.. code-block:: cpp

    [request = request]() -> void { /* Use a copy of the request. */ }
    [request = std::move(request)]() -> void { /* Own the moved request. */ }
    [owner = controller]() -> void { /* Retain the shared controller. */ }
    [weak = std::weak_ptr<Controller>{controller}]() -> void { /* Check later. */ }

These are capture alternatives, not a sequence to execute.
The first two require an owning request type if its contents must survive independently.
``std::move`` allows moving the value into the callable; it does not turn borrowed data into owned data.
The last line constructs a weak pointer while the lambda is created.
It does not retain the controller, even though its input is a strong pointer.

.. erbsland-in-practice-end::

Callback arguments have their own lifetime too.
An argument such as ``const List<IpAddress> &addresses`` borrows data for that call.
If a later operation needs those addresses, copy the list into owned state before returning, or use an owning value
supplied by the API.
Saving the address of the callback argument does not extend its lifetime.

Why Capturing ``this`` Can Crash
================================

``[this]`` stores a pointer to the current object.
It does not copy the object, create shared ownership, or postpone destruction.
If the callback accesses that pointer after the object is destroyed, the behavior is undefined: a crash is one possible
symptom, but apparent success is not evidence that the code is safe.

A default capture does not change this rule.
When ``[=]`` implicitly captures ``this`` to access a member, it still borrows the object; this implicit form is
deprecated in C++20. ``[&]`` also provides no lifetime extension.
``[*this]`` has a different meaning: it copies the object into the callable if the type is copyable.
That copy is not the original controller, and copying a controller's state is usually not the intended fix.

.. erbsland-in-practice-begin:: A Borrowed Controller Pointer

This fragment is unsafe if ``start()`` belongs to a controller that can disappear before delivery:

.. code-block:: cpp

    _lookup->events().onResolved([this](const el::List<el::IpAddress> &addresses) -> void {
        showAddresses(addresses); // Invalid if this controller has already been destroyed.
    });

Changing ``[this]`` to ``[=]`` does not repair it.
In the retained lookup application above, borrowing the application pointer is appropriate because the application
outlives dispatch.
A shorter-lived controller needs its own guarantee, even if the source is still retained elsewhere.

.. erbsland-in-practice-end::

There are two useful designs for shorter-lived recipients.
If its work is required, give the callback ownership of the recipient or keep an explicit operation owner until that
work finishes.
If its work is optional, let the recipient disappear and check for its presence when the callback runs.

Retaining a Required Recipient
------------------------------

Capturing a ``shared_ptr`` gives the callback its own ownership of the recipient.
The recipient remains alive while the callback is stored, not merely while the callback executes.
A source may keep a registered handler for reuse after an operation finishes, so completion alone does not necessarily
release its captures.
Retire the source or its handler when you no longer need that ownership.

Before choosing a strong capture, check whether the recipient owns the source or registration storing that callback.
The next section explains why a return ownership path changes the design.

.. erbsland-in-practice-begin:: A Report That Must Receive the Lookup Result

The :doc:`/demo_apps/event/lookup_required_recipient` program gives a separate ``AddressReport`` the responsibility for
printing resolved addresses.
The application owns the source; the report owns only its host value, with no pointer back to the source.
This makes a strong capture suitable:

.. erbsland-demo::
    :source: event/apps/LookupRequiredRecipient/LookupRequiredRecipientApp.hpp
    :show: f:begin, c:Application-owned operation(+0,+1)
    :exec: event/lookup_required_recipient
    :show-cmd-line:
    :sha256: 75a3accd5a767c67ef6d241c241a3aeb8206d267e7a4093c92f48778d2fd1f41

.. code-block:: cpp

    void begin() {
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        _lookup = loop()->get<el::Network>().createHostLookup();
        auto report = std::make_shared<AddressReport>(_host);
        _report = report; // A weak pointer lets the diagnostic observe destruction without preventing it.
        _lookup->events()
            .onResolved([report](const el::List<el::IpAddress> &addresses) -> void { report->onResolved(addresses); })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                _lookup.reset();
                quit(_exitCode);
            });
        _lookup->start(_host);
        report.reset(); // The callback has its own shared pointer to the report.
        el::io::printLine("Report retained by the callback: "_el, !_report.expired());
        if (_timeout.isZero()) {
            onTimeout();
        }
    }

    el::HostLookupPtr _lookup;                       ///< Application-owned operation; its callback retains the report.
    std::weak_ptr<AddressReport> _report;            ///< Non-owning diagnostic only.

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_required_recipient

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mReport retained by the callback: true
        Server address for 127.0.0.1: 127.0.0.1
        Report released after cleanup: true␛[0m

.. erbsland-demo-end::

Resetting the local ``report`` pointer does not destroy the report because the result handler has its own shared
pointer.
The diagnostic weak pointer observes that ownership without contributing to it.
After ``app.run()`` returns, the program checks that source cleanup has released the report too.
The report's job and lifetime are bounded by this one lookup.

.. erbsland-in-practice-end::

Allowing an Optional Recipient to Disappear
-------------------------------------------

A weak capture lets a callback detect that its recipient no longer exists.
Call ``lock()`` once at the point of use.
It either returns an empty pointer or gives you a temporary shared pointer that keeps the object alive until that local
pointer is destroyed.
Checking ``expired()`` and then using a separate raw pointer is not an equivalent lifetime guarantee.

If an optional recipient has disappeared, skipping its notification is a valid outcome.
Required results, operation completion, and application shutdown must still have an owner and a path forward.
Otherwise, making the callback safe can introduce a different bug: a task that waits forever for an optional recipient.

.. erbsland-in-practice-begin:: Turning Off Progress Without Losing the Result

The :doc:`/demo_apps/event/lookup_optional_recipient` program always retains resolved addresses in the application.
A separate ``LookupProgress`` object prints optional progress messages.
``--quiet`` releases that reporter after callback registration, before result dispatch, without cancelling the lookup.

.. erbsland-demo::
    :source: event/apps/LookupOptionalRecipient/LookupOptionalRecipientApp.hpp
    :show: f:begin, c:Required operation(+0,+3)
    :exec: event/lookup_optional_recipient
    :exec-2: event/lookup_optional_recipient --quiet
    :sha256: 55cbcdcf6bb1d39da82c3fbae9a83984d4f600706abcfc983481ca0ba04146e6

.. code-block:: cpp

    void begin() {
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        _lookup = loop()->get<el::Network>().createHostLookup();
        _progress = std::make_shared<LookupProgress>(_host);
        _lookup->events()
            .onResolved(
                [this, weak = std::weak_ptr<LookupProgress>{_progress}](
                    const el::List<el::IpAddress> &addresses) -> void {
                    _addresses = addresses; // Keep the required result independently of progress output.
                    if (const auto progress = weak.lock(); progress != nullptr) {
                        progress->onResolved(addresses);
                    }
                })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                _lookup.reset();
                for (const auto &address : _addresses) {
                    el::io::printLine("Server address: "_el, address);
                }
                quit(_exitCode); // Completion never depends on the progress reporter.
            });
        _lookup->start(_host);
        if (_quiet) {
            // Turn off an already connected reporter before any result can be dispatched.
            _progress.reset();
        }
        if (_timeout.isZero()) {
            onTimeout();
        }
    }

    el::HostLookupPtr _lookup;                       ///< Required operation, owned by the application.
    el::List<el::IpAddress> _addresses;              ///< Result copied out of the callback's borrowed argument.
    std::shared_ptr<LookupProgress> _progress;       ///< Optional reporter that can be released independently.
    bool _quiet{false};                              ///< Whether to discard progress reporting after setup.

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_optional_recipient

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mResolved 127.0.0.1 to 1 address(es).
        Server address: 127.0.0.1␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_optional_recipient --quiet

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mServer address: 127.0.0.1␛[0m

.. erbsland-demo-end::

The first run prints progress and the required result.
The second prints the required result alone.
The handler copies ``addresses`` into ``_addresses`` before considering optional output, so neither the borrowed result
argument nor the reporter controls result retention.
The final handler belongs to the application and completes the operation in both runs.

Here, the weak pointer is constructed from an existing shared pointer.
When a shared controller configures its own handlers, a capture initializer can instead use
``[weak = weak_from_this()]``:

.. code-block:: cpp

    auto callback = [weak = weak_from_this()](const el::List<el::IpAddress> &addresses) -> void {
        if (const auto self = weak.lock(); self != nullptr) {
            self->onResolved(addresses);
        }
    };

This alternative belongs in a shared controller's setup method; it uses the same lock-and-check body.

.. erbsland-in-practice-end::

``shared_from_this()`` and ``weak_from_this()`` come from ``std::enable_shared_from_this<Controller>``.
Use that public base when a shared controller needs to obtain pointers to itself.
Construct the controller with shared ownership first, then call a setup or ``start()`` method.
During its constructor, that shared ownership relationship is not yet established: ``shared_from_this()`` throws, and
``weak_from_this()`` yields an empty weak pointer.
Capturing that empty weak pointer does not make it valid when construction later completes.
Never create a new ``shared_ptr`` directly from ``this`` to try to establish the missing relationship; that creates a
separate owner that can delete the same object twice.

A successful weak lock proves that an object still exists, not that a result is still relevant to it.
A controller that can start a newer request may also need a request identifier to reject old results.
See :doc:`communicating_between_loops` for that separate concern.

Why Objects Never Get Destroyed
===============================

A strong capture can connect two otherwise reasonable ownership decisions into a cycle.
A controller owns its source, the source owns its callback, and the callback owns the controller.
Releasing the application's pointer leaves those objects owning each other:

.. mermaid::

    flowchart LR
        C["Controller"] -->|"owns source"| S["Source"]
        S -->|"stores callback"| H["Callback"]
        H -->|"captures shared controller"| C

Reference counting does not collect cycles.
The controller and source can remain allocated even after the operation completes and no application code needs them.
Repeatedly creating such operations can accumulate memory and other resources.
Moving the release into the controller's destructor does not fix this: the cycle is precisely what prevents that
destructor from running.

.. erbsland-in-practice-begin:: Observing and Breaking a Lookup Cycle

The :doc:`/demo_apps/event/lookup_ownership_cycle` program deliberately gives ``LookupController`` an owned source whose
handlers capture ``shared_from_this()``.
It starts after shared construction, so the captures are valid, but their strong ownership points back to the
controller:

.. erbsland-demo::
    :source: event/apps/LookupOwnershipCycle/LookupController.hpp
    :show: f:start, f:releaseSource, c:Owns handlers(+0,+1)
    :sha256: 25ae9aee583235489ae507aa2a77f40ac224843d6cf772efc3480d7dce53f2f2

.. code-block:: cpp

    void start(const el::EventTargetPtr &target, const el::Host &host, std::function<void(el::ExitCode)> onFinished) {
        _lookup = target->get<el::Network>().createHostLookup();
        _lookup->events()
            .onResolved([self = shared_from_this()](const el::List<el::IpAddress> &addresses) -> void {
                self->onResolved(addresses);
            })
            .onError([self = shared_from_this()](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                self->_exitCode = el::ExitCode::failure();
            })
            .onFinal([self = shared_from_this(), onFinished = std::move(onFinished)]() -> void {
                onFinished(self->_exitCode);
            });
        _lookup->start(host);
    }

    void releaseSource() noexcept { _lookup.reset(); }

    el::HostLookupPtr _lookup; ///< Owns handlers that strongly retain this controller: a deliberate cycle.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Operation result.

.. erbsland-demo-end::

The application releases its original controller pointer after starting the lookup.
It keeps a weak diagnostic pointer and arranges inspection on the next loop turn, after the final handler returns.
That separates retention by stored callbacks from the temporary ownership needed to execute a callback.

.. erbsland-demo::
    :source: event/apps/LookupOwnershipCycle/LookupOwnershipCycleApp.hpp
    :show: f:begin, f:inspectAndRelease, f:cleanup, c:Observes the cycle
    :exec: event/lookup_ownership_cycle
    :show-cmd-line:
    :sha256: b1e23d4d8409f4fefb27dcb311cfaa16dea4d3c3018a840a624887b8edcded97

.. code-block:: cpp

    void begin() {
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        auto controller = std::make_shared<LookupController>();
        _controller = controller;
        controller->start(loop(), _host, [this](const el::ExitCode code) -> void {
            // Inspect after the source's final handler has returned.
            loop()->invoke([this, code]() -> void { inspectAndRelease(code); });
        });
        controller.reset();
        el::io::printLine("External controller owner released."_el);
        if (_timeout.isZero()) {
            onTimeout();
        }
    }

    void inspectAndRelease(const el::ExitCode code) {
        if (_finished) {
            return;
        }
        _finished = true;
        el::io::printLine("Controller still alive after completion: "_el, !_controller.expired());
        if (const auto controller = _controller.lock(); controller != nullptr) {
            controller->releaseSource(); // Break controller -> source -> callback -> controller.
        }
        el::io::printLine("Controller released after breaking the cycle: "_el, _controller.expired());
        quit(code);
    }

    void cleanup() noexcept override {
        if (const auto controller = _controller.lock(); controller != nullptr) {
            controller->releaseSource();
        }
    }

    std::weak_ptr<LookupController> _controller; ///< Observes the cycle without adding another owning edge.

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_ownership_cycle

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mExternal controller owner released.
        Server address: 127.0.0.1
        Controller still alive after completion: true
        Controller released after breaking the cycle: true
        Controller released after cleanup: true␛[0m

.. erbsland-demo-end::

The controller is still alive after the lookup completes.
The application locks its diagnostic pointer to obtain a temporary owner, then explicitly releases the controller's
source and its stored callbacks.
After that temporary owner leaves scope, the controller is destroyed.
The application also breaks the cycle on deadline and in unconditional cleanup, so the demonstration does not leave its
intentionally retained objects behind.

.. erbsland-in-practice-end::

Designing Without the Cycle
---------------------------

The usual solution is to let an outside application own the controller while its callbacks refer back weakly.
That preserves the intended direction of ownership: the application decides how long the operation exists.
If the recipient must survive independently, separate it from source ownership, as with a result recipient that has no
pointer back to the source.

Explicit teardown is another option when your design already has a reliable owner responsible for ending the operation.
It can release the source, clear the stored handlers, or cancel the subscription that creates the cycle.
That owner must perform teardown while it can still reach the relevant objects, including on failure and cancellation.
Stopping an operation alone does not necessarily remove its stored callbacks.

The same reasoning applies to a controller-owned timer that captures its controller strongly, and to a subscription
stored in an observer whose callback captures that observer strongly.
Draw the complete ownership path, including registrations and callable captures, rather than considering each pointer in
isolation.
For timer-specific stop and destruction behavior, see :doc:`scheduling_work`.

Bindings make a lifetime choice easier to express, but do not remove that choice.
A member binding made with :cpp:func:`bindEvent() <erbsland::event::bindEvent>` retains its recipient and can
participate in a cycle.
:cpp:func:`bindWeakEvent() <erbsland::event::bindWeakEvent>` checks the recipient during delivery and skips an expired
recipient.
The binding mechanics and detailed examples belong to :doc:`event_bindings`.

.. _event-observer-lifetime:

Keeping a Subscription for as Long as You Need It
=================================================

An ``on...()`` method installs a handler owned by the source.
Where an API offers an ``add...()`` method, each call instead returns an independent
:cpp:class:`EventSubscription <erbsland::event::EventSubscription>` that controls one registration.
Keep that handle for the period when you want notifications.
Discarding it immediately cancels the registration at the end of the expression; keeping the publisher alive does not
repair a discarded subscription.

.. erbsland-in-practice-begin:: Reporting File-Server Statistics at Shutdown

The :doc:`/demo_apps/event/file_server` application serves files in an application part.
Its main function subscribes to part-state changes so it can print request statistics when the server starts stopping:

.. erbsland-demo::
    :source: event/apps/FileServer/main.cpp
    :sha256: f509ab68e97f5994e78fc8f69feba5d25ba6755447fe273efab14b1a974e52e8

.. code-block:: cpp

    /// Register an application part and let the default main manage its event thread and shutdown.
    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.info().setApplicationName("File Server"_el);
        app.enableServiceLifecycle();
        app.registerPart<ServerPart>();
        // Read a statistics snapshot on the application thread while the part is stopping.
        const auto stateSubscription = app.partManager()->events().addPartStateChanged(
            [&app](el::ApplicationPartIdentifierPtr, const el::ApplicationPartState state) -> void {
                if (state == el::ApplicationPartState::Stopping) {
                    const auto status = app.part<ServerStatus>();
                    el::io::printLine("Directory-index requests at shutdown: "_el, status->indexRequestCount());
                }
            });
        return app.run();
    }

.. erbsland-demo-end::

``stateSubscription`` is local, but its scope encloses ``app.run()``.
It therefore survives all the notifications needed by the shutdown report and is destroyed before ``app``.
The callback's borrowed ``&app`` is valid throughout that interval.

Replacing the declaration with ``(void)app.partManager()->events().addPartStateChanged(...)`` would discard the handle
immediately.
The server could still run, but its statistics callback would no longer be registered.
If this report belonged to a shorter-lived monitoring object, that object would normally store the subscription as a
member instead.

.. erbsland-in-practice-end::

Subscriptions are move-only.
Moving one transfers cancellation responsibility; the moved-from handle becomes inactive.
Move assignment also cancels the destination's previous registration.
Destroying the handle cancels it, and ``cancel()`` ends observation earlier when needed; repeated cancellation is
harmless.

The subscription does not keep the publisher alive.
Publisher destruction invalidates the registration even if your handle still exists.
``isActive()`` reports registration state, not operation completion or whether a callback is still running.

Cancellation prevents future dispatch through that registration, but a dispatch already in progress may still execute or
finish after ``cancel()`` returns.
Nor does cancellation retract separate work that an earlier callback already queued, such as a bound delivery on another
loop.
A weak recipient check or an explicit lifetime guarantee is still needed there.
Use :doc:`shutting_down_event_work` when you need to coordinate cancellation, completed callbacks, and thread shutdown.

Borrowing an Event Editor
=========================

A source's ``events()`` returns a reference to an editor owned by that source.
The reference provides access to configure callbacks; it is not another owning handle.
This is why the API returns a reference rather than an independently retained ``shared_ptr`` to an editor.
Storing an editor reference as a member or capturing it in later work can hide the fact that your code no longer owns
the source it intends to configure.
When that source dies, the editor reference becomes invalid too.

A short local reference is fine while an owning source pointer remains valid.
For later edits, store the source pointer and obtain its editor again on the owner loop, unless that editor explicitly
permits access from other threads.

.. erbsland-in-practice-begin:: Editing the Retained Lookup

In the application that owns ``_lookup``, this temporary reference is valid during owner-loop setup:

.. code-block:: cpp

    auto &editor = _lookup->events();
    editor.onResolved([](const el::List<el::IpAddress> &addresses) -> void {
        el::io::printLine("First server address: "_el, addresses.first());
    });

The member ``_lookup`` supplies the ownership.
Saving ``editor`` elsewhere would add no ownership, and it could not be used after ``_lookup`` releases the last source
pointer.
When a later operation needs to change the handler, access ``_lookup->events()`` again while that source is retained.

.. erbsland-in-practice-end::

Generic code can obtain an owning source pointer through
:cpp:func:`EventEditor::source() <erbsland::event::EventEditor::source>` while the editor is still valid.
It cannot use that method to rescue an editor whose source has already died.
:cpp:func:`EventEditor::target() <erbsland::event::EventEditor::target>` retains only the callback target, which does not
solve source ownership.
