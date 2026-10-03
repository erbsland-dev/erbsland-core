.. index::
    single: Cooperative Stopping
    single: Cancellation; Stop Tokens

********************
Cooperative Stopping
********************

``StopSource`` and ``StopToken`` share one irreversible atomic stop request without requiring standard-library
stop-token support.
Copy a source to share signalling authority; copy its token to share read-only observation.
Both handles can be used across threads, and tokens remain valid after their sources are destroyed.

``StopSource::requestStop()`` returns true only for the first request and never waits for observers.
``isStopRequested()`` on either handle observes that request.
Destroying a source does not request stopping.
A default token never reports a stop request; a moved-from source cannot request stopping.

.. code-block:: cpp

    auto source = el::StopSource{};
    const auto token = source.stopToken();
    source.requestStop();
    const auto stopping = token.isStopRequested(); // true

The signal is cooperative: it does not interrupt threads or system calls.
There is no reset, callback registration, or interruptible-wait API.
``WorkerPool`` owns one source for all its operations and passes tokens to callables that accept them; see
:doc:`/reference/util/worker_pool`.

Interface
=========

.. doxygenclass:: erbsland::util::StopSource
    :members:
.. doxygenclass:: erbsland::util::StopToken
    :members:
