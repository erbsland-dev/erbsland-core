.. index::
    single: Worker Pools
    single: Parallel Processing

************
Worker Pools
************

``util::WorkerPool`` runs independent callables on owned, joinable threads.
It supports standard futures without an event loop and optional typed completion callbacks on one owner loop.
Pools are independent of Core's internal I/O and coroutine workers, so application work can wait for those services
without consuming their execution capacity.

Procedural Work
===============

``submit(work)`` returns ``std::future<R>`` for one value or void result.
``map(values, work)`` returns one future carrying an ordered ``List<R>``; ``forEach(values, work)`` returns one void
future.
Call ``get()`` to consume the result or rethrow its operational error.
Discarding a future does not cancel work; retain and observe it when failures matter.

.. erbsland-demo::
    :source: util/WorkerPool/Procedural.cpp
    :exec: util/worker_pool --demo Procedural
    :sha256: c55c88d830f0f9d225c39dcaf582fe1ab93ae949fe6b1bb4d6825c1c1c71175c

.. code-block:: cpp

    /// Parallel population projections with one ordered result list.
    /// WorkerPool owns its threads and drains accepted work on destruction. A map call
    /// snapshots the input and returns one standard future for the complete operation.
    void procedural() {
        const auto workers = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2));
        const auto populations = el::List<double>{100.0, 250.0, 500.0};

        auto projected = workers->map(populations, [](const double &initial, el::StopToken stop) -> double {
            auto population = initial;
            for (auto year = 0; year < 20 && !stop.isStopRequested(); ++year) {
                population += 0.15 * population * (1.0 - population / 1000.0);
            }
            return population;
        });

        // Results retain input order even when individual projections finish out of order.
        el::stdOut()->printLine("Habitat: "_el, "duingebied"_el);
        for (const auto population : projected.get()) {
            el::stdOut()->printLine(el::StringFormat{"Projected population: {:.1f}"_el}.build(population));
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mHabitat: duingebied
        Projected population: 675.9
        Projected population: 874.4
        Projected population: 958.5␛[0m

.. erbsland-demo-end::

A callable may take no arguments or a ``StopToken``; collection callables take a const item reference and optionally a
final token.
The token-taking form wins when both forms are callable.
Single jobs support move-only callables and result values; reference results are excluded.
Move-only completion callbacks are supported as well.

Bulk Operations
===============

Bulk calls copy a finite readable range into owned storage before returning.
They never retain borrowed iterators or views.
Input elements and mapped results must be copy-constructible values.
Workers invoke one shared callable through const access, so the caller must synchronize any shared mutable state
referenced by that callable.

``map()`` preserves input order, independent of execution order.
``forEach()`` performs side effects and does not modify the original collection.
Empty inputs complete successfully; event completion remains deferred.

One bulk call consumes one admission slot.
Items are claimed incrementally without per-item futures or queue entries, and other operations can receive workers
between claims.
The first operational error stops the batch from starting more items; running items finish before its error is
published.
Other operations continue.
Failed and cancelled batches never expose partial result lists.

Optional Owner-Loop Delivery
============================

``WorkerPool`` is an ``EventSource`` with a stable ``WorkerPoolEventEditor``.
Use ``WorkerPool::create()`` for a pool whose owner is selected by its first event use.
To select the owner explicitly, obtain its ``Utilities`` frontend and create the pool inside that loop:

.. code-block:: cpp

    loop->invoke([loop, this, options]() -> void {
        _workers = loop->get<el::Utilities>().createWorkerPool(options);
        _workers->events().onFinal([this]() -> void { finishWorkflow(); });
    });

``EventTarget::get()`` returns a reference, so the factory call uses a dot.
As with ``System::createSubprocess()``, creation through the frontend must run on its owner loop.
Each call creates an independent pool; the frontend does not retain it.
Retain the returned pool while its work and completion handlers are needed.

Like ``Subprocess``, the first ``events()`` call binds the current loop permanently.
Callback-form submissions bind it implicitly when needed.
Perform binding, handler editing, and callback-form submission inside that owner loop.
Procedural submissions remain thread-safe and can coexist with event operations.

``submit(work, completed[, failed])``, ``map(values, work, completed[, failed])``, and
``forEach(values, work, completed[, failed])`` deliver through callbacks instead of futures.
The successful callback receives a result by value, or no argument for void.
Every callback executes on the owner loop, never on a worker.

.. erbsland-demo::
    :source: util/WorkerPool/EventDelivery.cpp
    :exec: util/worker_pool --demo EventDelivery
    :sha256: 40e6689ade0fbe65022ff812680d9aaba5358be53217f4633b778cdd3ad02166

.. code-block:: cpp

    /// Deliver parallel projections on one event loop without waiting inside its callbacks.
    /// Keep the pool alive until its final event; quit closes admission while accepted
    /// computations and their completion callbacks finish normally.
    void eventDelivery() {
        const auto loop = el::EventLoop::create();
        auto workers = el::WorkerPoolPtr{};

        loop->invoke([&]() -> void {
            workers = loop->get<el::Utilities>().createWorkerPool(el::WorkerPoolOptions{}.setWorkerCount(2));
            workers->events().onFinal([&]() -> void { loop->quit(); });
            workers->map(
                el::List<double>{100.0, 250.0, 500.0},
                [](const double &initial) -> double {
                    auto population = initial;
                    for (auto year = 0; year < 20; ++year) {
                        population += 0.15 * population * (1.0 - population / 1000.0);
                    }
                    return population;
                },
                [](el::List<double> populations) -> void {
                    el::stdOut()->printLine("Habitat: "_el, "duingebied"_el);
                    for (const auto population : populations) {
                        el::stdOut()->printLine(el::StringFormat{"Projected population: {:.1f}"_el}.build(population));
                    }
                });
            workers->quit();
        });

        loop->run();
        workers->join();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mHabitat: duingebied
        Projected population: 675.9
        Projected population: 874.4
        Projected population: 958.5␛[0m

.. erbsland-demo-end::

The editor's ``onError(std::exception_ptr)`` is the fallback for operational task failures.
A per-operation ``failed`` callback overrides it.
Without either handler, the exception is rethrown during dispatch and follows the owner's configured error policy.
Exceptions from completion handlers also follow that loop's normal policy.

Cancellation invokes the operation's ``failed`` handler with ``WorkerCancelledError`` when supplied; otherwise it
invokes ``onCancelled()`` or is silently ignored.
``onIdle()`` reports that accepted work and outcome callbacks have drained.
New submissions suppress stale idle notifications.
``onFinal()`` runs once after closed-pool workers finish and preceding outcomes have been delivered.
Binding does not replay past idle or final transitions.

Each ``on...()`` replaces one handler; an empty handler clears it.
Source and target access follow the common
:doc:`/reference/event/event_system` editor rules. Retain the source while callbacks are wanted; handlers must not strongly capture their
source.
Pool destruction suppresses callbacks that have not begun.

Completion delivery reserves one internal continuation per pool, independent of ordinary event-queue limits and overflow
policies.
Initial binding can fail with ``EventQueueFullError`` if no continuation can be reserved.
Owner-loop shutdown suppresses undelivered outcomes and releases their admission without delaying computations or
procedural waits.

Stopping and Lifetime
=====================

The pool owns a ``StopSource`` and supplies its read-only ``StopToken`` to cooperative work.
See
:doc:`/reference/util/cooperative_stopping`. ``stopToken()`` also exposes the token directly. Stopping never forcibly
interrupts a running callable or a blocking system call.

* ``wait()`` waits for queued/running work, release of its work captures, and future publication; the pool stays open.
* ``quit()`` closes admission and drains accepted work without requesting cancellation or waiting.
* ``stop()`` closes admission, requests the shared token, and cancels unfinished operations without waiting.
* ``join()`` waits for worker termination without initiating shutdown.
* Destruction closes admission, drains accepted work, and joins workers; it never dispatches callbacks.

``quit()`` and ``stop()`` are idempotent.
``stop()`` can escalate graceful shutdown.
A closed pool cannot restart.
Success committed before ``stop()`` remains success, including results waiting for event delivery.
Failure and cancellation select one synchronized first terminal decision.
Running items still release their captures before that decision becomes observable through a future or completion
callback.

Blocking waits do not pump the owner loop or wait for callback delivery.
They can therefore finish with undelivered event results still consuming admission.
Workers must not synchronously wait for dependent work on their own pool.
Calling ``wait()`` or ``join()`` from a pool worker raises ``LogicFailure``; releasing the final pool owner on its own
worker terminates.
Do not create a dependency where a joining thread is required to complete a worker's blocking request.

Capacity and Failures
=====================

``WorkerPoolOptions`` is copied at creation.
``workerCount()`` defaults to zero, selecting hardware concurrency with a minimum of one.
The effective count is available from the pool and stays fixed.
``maximumPendingTasks()`` defaults to zero for unlimited admission.
A finite limit counts whole operations until future publication or event dispatch/discard; it does not limit snapshot or
result byte sizes.
Submissions never wait for capacity.

``WorkerPoolFullError`` reports exhausted configured admission.
``WorkerPoolClosedError`` reports a closed pool or closed event delivery.
Rejected work never executes.
``isAccepting()`` is a synchronized snapshot of procedural admission, independent of capacity and event-owner shutdown.

Only ``err::RuntimeError`` exceptions escaping work are transported.
Other exceptions, including programming and resource failures, terminate at the worker boundary.
Allocation and native-thread creation failures during submission or construction propagate after cleanup; they are not
converted into operational task failures.
Snapshot-copy errors occur synchronously before admission.
Destruction and joins do not consume future results or rethrow task errors.

Interface
=========

.. doxygenclass:: erbsland::util::Utilities
    :members:
.. doxygenclass:: erbsland::util::WorkerCancelledError
    :members:
.. doxygenclass:: erbsland::util::WorkerPool
    :members:
.. doxygenclass:: erbsland::util::WorkerPoolClosedError
    :members:
.. doxygenclass:: erbsland::util::WorkerPoolEventEditor
    :members:
.. doxygenclass:: erbsland::util::WorkerPoolFullError
    :members:
.. doxygenclass:: erbsland::util::WorkerPoolOptions
    :members:
