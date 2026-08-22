.. index::
    single: HTTP Client
    single: HTTP Client Session
    single: HTTP Response Aggregation

***********
HTTP Client
***********

Session and Requests
====================

``HttpClientSession`` is an active, event-loop-owned HTTP/1.1 and HTTPS client created by
``Network::createHttpClientSession()``.
It captures limits, TLS policy, default fields, and fallback callbacks when a prepared ``HttpClientRequest`` is
submitted.
Convenience operations create, submit, and return retained GET, HEAD, or fixed-body POST requests.

The session bounds concurrent and pending operations and retained fixed request bodies.
``close()`` stops admission and drains submitted requests before emitting ``onClosed`` and ``onFinal``.
``abort()`` cancels queued and active requests and emits ``onFinal``.

Redirects
=========

``HttpClientRedirectOptions`` defaults to following at most ten redirects on any host while rejecting HTTPS-to-HTTP
downgrades.
The session policy is captured when a request is submitted; a prepared request can override it.
``onRedirect`` receives the response head, source URL, optional resolved target, followed count, and a machine-readable
hard constraint.
A request callback overrides the captured session fallback and returns ``Follow``, ``ReturnResponse``, or ``Reject``.

The client follows 301, 302, 303, 307, and 308 responses.
POST changes to GET for 301 and 302; 303 changes every method except HEAD to GET; and 307 and 308 preserve the method
and body.
Empty and fixed bodies are replayable.
A streamed body can only follow when the status rewrite discards it.
Loop, limit, malformed-target, downgrade, host-policy, and body-replay guards cannot be bypassed by a callback.
``ReturnResponse`` exposes the retained 3xx response through the ordinary response policy, while a blocked follow or
explicit rejection reports ``HttpRedirectFailure``.

Relative targets resolve against the current hop and inherit a source fragment when ``Location`` has none.
Cross-origin hops permanently discard authorization, proxy authorization, caller cookies, origin, and referrer fields.
Host, framing, and automatic cookies are regenerated for every hop.
``HttpClientRequest::url()`` remains the submitted URL; the final response exposes ``effectiveUrl()`` and
``redirectCount()``.

Cookie Handling
===============

Every session owns one stable ``HttpCookieJar``.
Automatic cookie handling is enabled by default: valid ``Set-Cookie`` fields are stored before redirect policy runs, and
matching cookies generate the request ``Cookie`` field.
A caller ``Cookie`` field therefore fails submission while automation is enabled.
Disable automation explicitly when an application must own that field.

The jar supports exact inspection, insertion, replacement, removal, and clearing.
It implements bounded domain and path matching, default paths, host-only and Secure cookies, HttpOnly inspection,
Max-Age precedence, expiry, ``__Secure-`` and ``__Host-`` prefix rules, and deterministic sending and LRU eviction.
SameSite is retained for inspection but browser navigation semantics are not enforced.
Defaults are 4 KiB per cookie, 180 cookies per registrable domain, and 3,000 globally.
Registrable-domain policy uses the bundled complete ICANN and private Public Suffix List; IP-address cookies remain
exact-host-only.

Connection Reuse
================

Sequential HTTP/1.1 reuse is enabled by default.
The session retains at most one idle connection per origin, eight idle connections globally, for 30 seconds and at most
1,000 transactions per physical connection.
Concurrent logical requests remain independent and may create additional same-origin connections; only one completed
idle connection is kept.

Origin and captured transport/TLS configuration generations isolate reusable connections.
Unsolicited input, closure, errors, retained bytes, expiry, or a transaction cap evicts a connection.
A stale reused connection is retried at most once only for GET, HEAD, OPTIONS, or TRACE with an empty or fixed body and
before any informational or final response was observed.
Session ``close()`` drains leased requests without accepting their returned connections; ``abort()`` terminates both
active work and idle connections.

Response Handling
=================

``onResponse``, ``onTextResponse``, and ``onJsonResponse`` select one bounded automatic aggregate.
Text is decoded as strict UTF-8 and JSON is parsed before its callback runs.
``HttpClientResponseOptions`` controls the aggregate limit and optional media-type patterns.

``onResponseHead`` is the low-level alternative.
Its retained ``HttpClientResponse`` must select streamed delivery, bounded byte, text, or JSON aggregation, body
rejection, or pumping into a ``ByteOutputStream``.
Streaming supports pause and resume, while output-stream pumping pauses input around each bounded worker write.
The ``Path`` overload creates a collision-safe temporary file in the destination directory on a bounded worker, reports
committed progress, flushes and closes it, and atomically replaces a regular-file destination only after the complete
response succeeds.
Cancellation, deadlines, write failures, and commit failures remove the temporary file and preserve an existing
destination.

HTTPS Policy
============

``HttpClientTlsOptions`` selects the ``http/client`` configuration label by default.
HTTPS requests resolve the corresponding immutable TLS configuration, validate the server through its explicit trust
anchors, and offer only ``http/1.1`` through ALPN.

See :doc:`/topics/network/using-http-clients` for request preparation, response policies, streaming, errors, and
lifecycle examples.

Interface
=========

.. doxygenclass:: erbsland::network::HttpClientBodyProgress
    :members:
.. doxygenenum:: erbsland::network::HttpClientRedirectAction
.. doxygenenum:: erbsland::network::HttpClientRedirectConstraint
.. doxygenclass:: erbsland::network::HttpClientRedirectContext
    :members:
.. doxygenenum:: erbsland::network::HttpClientRedirectHostPolicy
.. doxygenclass:: erbsland::network::HttpClientRedirectOptions
    :members:
.. doxygenclass:: erbsland::network::HttpClientRequest
    :members:
.. doxygenclass:: erbsland::network::HttpClientRequestEventEditor
    :members:
.. doxygentypedef:: erbsland::network::HttpClientResponseFn

.. doxygentypedef:: erbsland::network::HttpClientTextResponseFn

.. doxygentypedef:: erbsland::network::HttpClientJsonResponseFn

.. doxygentypedef:: erbsland::network::HttpClientResponseHeadFn

.. doxygentypedef:: erbsland::network::HttpClientInformationalResponseFn

.. doxygentypedef:: erbsland::network::HttpClientRedirectFn

.. doxygentypedef:: erbsland::network::HttpClientBodyProgressFn

.. doxygentypedef:: erbsland::network::HttpClientErrorFn
.. doxygenclass:: erbsland::network::HttpClientResponse
    :members:
.. doxygenclass:: erbsland::network::HttpClientResponseEventEditor
    :members:
.. doxygenclass:: erbsland::network::HttpClientResponseOptions
    :members:
.. doxygenclass:: erbsland::network::HttpClientSession
    :members:
.. doxygenclass:: erbsland::network::HttpClientSessionEventEditor
    :members:
.. doxygenclass:: erbsland::network::HttpClientSessionOptions
    :members:
.. doxygenclass:: erbsland::network::HttpClientTlsOptions
    :members:
.. doxygenclass:: erbsland::network::HttpCookie
    :members:
.. doxygenclass:: erbsland::network::HttpCookieInsertionOptions
    :members:
.. doxygenclass:: erbsland::network::HttpCookieJar
    :members:
.. doxygenclass:: erbsland::network::HttpCookieJarOptions
    :members:
