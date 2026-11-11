..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************************************
Server Lookup with an Ownership Cycle
*************************************

This application deliberately builds a cycle between a lookup controller, its source, and its callbacks.
It completes a real lookup, observes that the controller is still alive, then explicitly dismantles the cycle.
It also releases the cycle on failure or deadline.
The general prevention strategies are explained in :doc:`/topics/event/callback_lifetimes`.

Creating the Return Ownership Path
==================================

``LookupController`` is constructed in shared ownership before ``start()`` uses ``shared_from_this()``.
Its owned source stores handlers that retain the controller:

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

The operation finishing does not clear those stored handlers.
Consequently, dropping the outside controller pointer cannot destroy the controller.
Putting ``releaseSource()`` only in its destructor would not help, because the cycle prevents destruction.

Observing and Dismantling the Cycle
===================================

The application keeps only a weak diagnostic pointer after setup.
Its completion callback queues inspection for the next loop turn, after the final handler has returned:

.. erbsland-demo::
    :source: event/apps/LookupOwnershipCycle/LookupOwnershipCycleApp.hpp
    :show: f:begin, f:inspectAndRelease, c:Observes the cycle
    :exec: event/lookup_ownership_cycle
    :show-cmd-line:
    :sha256: bc4b76a54d06840da2b031ba4d5f2e8f608a7a3b36dfc3dc510afe9df7f7138a

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

The temporary locked pointer keeps the controller valid while ``releaseSource()`` destroys its source and handlers.
When that local pointer goes out of scope, the weak pointer expires.
The post-run diagnostic confirms that application cleanup leaves no controller behind.

Cleanup Without a Final Notification
====================================

The outside application can also dismantle the cycle without receiving completion.
Its deadline and unconditional cleanup use the same explicit release:

.. erbsland-demo::
    :source: event/apps/LookupOwnershipCycle/LookupOwnershipCycleApp.hpp
    :show: f:onTimeout, f:cleanup
    :exec: event/lookup_ownership_cycle --timeout-ms 0
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 6629f59d0425399e03be8886b31c63b4d0a400bf6e46c4dc3b7da0cb113f989f

.. code-block:: cpp

    void onTimeout() {
        if (_finished) {
            return;
        }
        _finished = true;
        cleanup(); // Also break the cycle if the expected completion never arrives.
        el::io::printLine("Deadline reached; controller released: "_el, _controller.expired());
        quit(el::ExitCode::failure());
    }

    void cleanup() noexcept override {
        if (const auto controller = _controller.lock(); controller != nullptr) {
            controller->releaseSource();
        }
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_ownership_cycle --timeout-ms 0

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mExternal controller owner released.
        Deadline reached; controller released: true
        Controller released after cleanup: true␛[0m

.. erbsland-demo-end::

The host defaults to loopback, and ``--timeout-ms`` defaults to one second.
Zero aborts at the end of setup before the numeric result; ``--help`` describes both options.
No dangling access is executed, and no intentionally retained cycle is left behind.
