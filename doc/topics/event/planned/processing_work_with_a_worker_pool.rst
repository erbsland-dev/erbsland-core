..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Worker Pool

********************************************
(planned) Processing Work with a Worker Pool
********************************************

.. erbsland-draft:: Planned topic; the tutorial has not been written yet.

A :cpp:class:`util::WorkerPool <erbsland::util::WorkerPool>` schedules independent jobs across reusable worker threads.
Consider it before creating several event threads just to distribute computations: jobs usually need a place to run, not
a dedicated loop of their own.

A pool can be used with or without an event loop.
In an event-driven application, completion callbacks return results to an owner loop while that loop remains responsive.
Procedural callers can use futures to wait for results instead.
A future wait on the main thread is unsuitable when the required completion path itself needs that thread's loop.

This planned tutorial will cover choosing a pool, submitting jobs, receiving results on an event loop, and completing or
cancelling a batch.
For the available APIs and their current contracts, see :doc:`/reference/util/worker_pool`.
For dedicated loop ownership, see :doc:`../processing_work_on_event_threads`.
