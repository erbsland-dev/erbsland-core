..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Worker Pools
    single: Worker Pools; Delivering Results

**********************************
Processing Work with a Worker Pool
**********************************

Suppose a callback receives a batch of values and each value needs a calculation.
As long as that callback is calculating, its loop cannot handle input or deliver other results.
A :cpp:class:`WorkerPool <erbsland::util::WorkerPool>` lets the callback submit the batch and return.
The workers compute the results, and the loop receives them together when the batch is ready.

Choosing the Unit of Work
=========================

Look at what one calculation needs.
If it can take an input value, use local working state, and return a value, any available worker can run it.
That gives the pool freedom to start the next item as soon as a worker finishes.
A fixed set of workers can therefore handle a batch much larger than the thread count.

Use :cpp:func:`submit() <erbsland::util::WorkerPool::submit>` for one job, or
:cpp:func:`map() <erbsland::util::WorkerPool::map>` when you want one result per input.
A map takes a snapshot of the input collection before returning.
Each worker reads an item from that snapshot, and the pool places the result at the corresponding position in the
output.
The first result in the list belongs to the first input even if another worker finished earlier.

A component that owns a live event source has a different requirement.
Its configuration, notifications, and state changes may all need to run on the same loop over time.
:doc:`processing_work_on_event_threads` describes how to give such a component a dedicated loop.
A worker-pool job can still do calculations for that component and return values to it.

Receiving Results on the Owner Loop
===================================

Create the pool inside a callback on the loop that should receive its results.
That callback can obtain the loop's :cpp:class:`Utilities <erbsland::util::Utilities>` interface, call
:cpp:func:`createWorkerPool() <erbsland::util::Utilities::createWorkerPool>`, and submit the batch.
Keep the returned pool in an owner that survives submission and delivery.
A local pointer in the setup callback alone would let the pool be destroyed as soon as that callback returns.

The two callables passed to ``map()`` have different jobs.
The first runs on pool threads and may run concurrently for several items.
Keep its working data local and leave loop-owned state to the completion callback.
The second callable receives the whole result list by value on the owner loop.
There you can update the component that requested the work.
While the workers calculate, the loop can dispatch its other callbacks.

.. erbsland-in-practice-begin:: Projecting Three Populations

Here two workers calculate twenty years of growth for three starting populations.
The outer function retains the pool while the loop runs.
Inside the map, the first lambda holds one projection in a local variable.
The second writes the results for all three starting values in their original order.

.. erbsland-demo::
    :source: util/WorkerPool/EventDelivery.cpp
    :show: f:eventDelivery
    :exec: util/worker_pool --demo EventDelivery
    :sha256: 202de832e8cd5d5e320305c8ccb4fb0cd2fc42fe1b484e0f2b4f3ccd6a029c2e

.. code-block:: cpp

    void eventDelivery() {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
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
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
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

The first printed projection belongs to the starting population of 100. Only the result callback prints these numbers,
so their order tells you how to associate a result with its input.
The program can produce the same output under different worker schedules.

.. erbsland-in-practice-end::

Finishing a Batch and Closing the Pool
======================================

A batch result belongs to one submission.
Its completion handler can submit another batch if the pool should stay open.
When you know you have submitted the last one, call :cpp:func:`quit() <erbsland::util::WorkerPool::quit>` on the pool.
It closes admission immediately and lets accepted work finish.
Keep dispatching the owner loop so that it can deliver those outcomes.

Register :cpp:func:`onFinal() <erbsland::util::WorkerPoolEventEditor::onFinal>` before requesting shutdown.
It runs once the closed pool's workers have finished and earlier outcomes have been delivered.
If that pool was the loop's last responsibility, the final handler can call ``quit()`` on the *loop*.
If the loop serves other components, the final handler can simply record that this pool has finished.
The owner can then :cpp:func:`join() <erbsland::util::WorkerPool::join>` the closed pool.
A join waits for worker termination and leaves callback delivery to the loop.

When work throws an Erbsland :cpp:class:`RuntimeError <erbsland::err::RuntimeError>`, the batch can finish through a
per-operation failure callback.
A failed map delivers the error after the running items finish and discards partial results.
Without a per-operation handler, a pool-wide ``onError()`` handler can receive the exception.
An unhandled operational error reaches the loop's error policy.
For a caller driving its own loop, the ``Quit`` policy and a check of ``takeError()`` after dispatch give that error a
path back to the caller.

To abandon unfinished work, call :cpp:func:`stop() <erbsland::util::WorkerPool::stop>` on the pool.
It closes admission and requests cooperative cancellation.
A long-running calculation can accept a :cpp:class:`StopToken <erbsland::util::StopToken>` and check it between bounded
steps so its worker can return promptly.
An operation cancelled this way reaches its failure callback with
:cpp:class:`WorkerCancelledError <erbsland::util::WorkerCancelledError>`, or the pool's ``onCancelled()`` handler if
installed.
An outcome already committed as a success can still arrive.
See :doc:`/reference/util/worker_pool` for cancellation, admission limits, and the precise failure contracts.

Consuming the Same Results from Procedural Code
===============================================

If the next step in your program needs the complete list and the calling thread is free to wait, create a pool with
``WorkerPool::create()``.
Call ``map()`` with just the inputs and the calculation.
It returns one future for the batch.
Calling ``get()`` waits for the list and also gives an operational failure a place to propagate.
This form needs no event loop.
On a thread that must continue dispatching events, use the callback form so waiting for a result cannot hold up
delivery.

.. erbsland-in-practice-begin:: Reading the Projections from a Future

The inputs and growth calculation are the same.
Here the caller reaches the printing loop in its ordinary control flow and reads the list from the future.

.. erbsland-demo::
    :source: util/WorkerPool/Procedural.cpp
    :show: f:procedural
    :exec: util/worker_pool --demo Procedural
    :sha256: f4190ed55b1e851ee59b85d53ba0e7bbeb2c1f7d65851a0ef5279c29dc7654b7

.. code-block:: cpp

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

The token gives each projection a chance to finish early if the pool receives a stop request.
In this run all three complete, and the caller can release the pool after printing the list.

.. erbsland-in-practice-end::
