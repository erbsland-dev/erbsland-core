.. index::
    single: HTTP Server
    single: HTTP Routing
    single: HTTP Server Session
    single: Cookie Session Manager

***********
HTTP Server
***********

Server Lifecycle
================

``HttpServer`` is an event-loop-owned HTTP/1.1 server created by ``Network::createHttpServer()``.
Configure transport, HTTP, TLS, and session options while it is inactive, register routes through its event editor, and
then bind it to an ``IpEndpoint``.
Without ``enableTls()`` it accepts plaintext HTTP.
Calling ``enableTls()`` selects the ``http/server`` identity through the normal configuration fallback chain and forces
the single ALPN protocol ``http/1.1``.
``HttpServerTlsOptions`` exposes only the identity label, exact SNI mappings, handshake capacity, and handshake
deadline; transport, cipher, buffer, and closure details remain safe server-owned defaults.

The effective endpoint becomes available after ``onListening``.
``close()`` stops accepting, disables keep-alive reuse, drains committed responses, and then reports ``onClosed`` and
``onFinal``.
``abort()`` terminates listener and connections immediately.

Routing and Requests
====================

Routes are additive across ``onRequest``, ``onTextRequest``, ``onJsonRequest``, and ``onRequestHead`` and are searched
in registration order.
Session routes are searched before server routes.
Patterns match the decoded NFC path while ignoring the exact query.
``{name}`` captures one complete nonempty segment and a final ``{*name}`` captures the remaining segments.
An explicit ``HEAD`` route wins; otherwise a matching ``GET`` route handles ``HEAD`` while response body bytes are
suppressed.

``HttpServerRequest`` retains the immutable request head, decoded path, exact query, route parameters, endpoints,
concrete connection, and selected logical session.
Ordinary byte, text, and JSON routes aggregate automatically under a one-MiB default and optional route-specific
Content-Type policy.
Text routes require strict UTF-8 and JSON routes parse one complete value before invoking application code.
``onRequestHead`` is the low-level family for selecting streaming, bounded aggregation, or rejection manually.
Fixed response helpers, response start, and response finish are queued by the framework; only streamed ``sendBody``
reports atomic back-pressure.
A request retained beyond its transaction remains safe: void response operations do nothing and ``sendBody`` reports
closed.

Static Content
==============

``HttpStaticContentHandler`` is the extensible source interface.
Its ``hasPath()`` probe and ``getContent()`` factory receive validated, decoded-NFC relative ``Path`` values and run on
bounded workers, potentially concurrently.
A positive probe is authoritative: failures while creating or opening that content do not fall through to another
overlay.
The returned ``HttpStaticContent`` reports its exact length and retained-memory cost before its one-shot ``open()``
creates a byte input stream.

``HttpStaticFileHandler`` exposes one filesystem tree, while ``HttpStaticResourceHandler`` exposes exact paths below one
compiled-resource identifier.
Their public classes are abstract interfaces and their factories create built-in final implementations.
Handlers are added with ``HttpServer::addStaticContentHandler()`` while the server is inactive.
At ``start()``, the server freezes and retains the original handler pointers, then stable-sorts a copied pointer vector
by descending priority.
Configuration and shared media mappings reject mutation while any server using them is starting or active; the freeze is
reference-counted and released after terminal shutdown and outstanding worker use.

Session and server routes always take precedence.
A dynamic path match also suppresses static content when its method does not match or it is a fallback.
Static handlers accept ``GET`` and ``HEAD``; another method for existing content returns ``405`` with
``Allow: GET, HEAD``.
Prefixes match complete decoded NFC segments.
Only a negative probe continues to the next matching handler, which permits deterministic built-in or custom overlays.

Paths without a trailing slash are probed exactly and never infer a directory or redirect.
For a path ending in ``/``, the server appends each configured index filename and probes it in order; the defaults are
``index.html`` and ``index.htm``.
Filesystem traversal rejects links, non-regular files, unsafe decoded segments, Windows device and alternate-stream
spellings, and containment changes.
Candidates are resolved with ``Path`` below the resolved root and opened through ``Path::content()`` with restrictive
input-stream symlink handling.
Filesystem roots are validated and canonicalized synchronously by ``HttpStaticFileHandler::create()``.
Probes, content creation, one-shot opening, and stream reads run on the blocking worker bridge; completions return to
the owning event loop and output retains at most one back-pressured block.

``HttpMediaTypeMapping::defaultMapping()`` supplies common web suffixes and an ``application/octet-stream`` fallback.
Use ``create()`` for an empty mapping or ``defaultMapping()->copy()`` for an editable built-in clone.
Matching is ASCII-case-insensitive and longest-suffix-first.
Resource handlers retain an explicit ``ResourcesConstPtr`` or borrow the application-lifetime manager, and enforce a
one-MiB per-resource logical-size default before loading data.
They expose resource bytes through ``ByteBlockInputStream`` without another content copy.

Sessions
========

Without a manager, each connection receives one anonymous ``HttpServerSession`` reused by its sequential requests.
Session data is an application-defined ``HttpSessionData`` pointer.
A synchronous ``HttpServerSessionManager`` can select identified sessions shared by multiple connections and attach
bounded fields to the eventual response.

``HttpCookieSessionManager`` provides opaque 256-bit identifiers in a host-only, ``HttpOnly`` cookie.
Its bounded server-side registry defaults to a 30-minute idle lifetime, a 24-hour absolute lifetime, and 10,000
sessions.
Unknown identifiers never become session identities.
Expiry and least-recently-used eviction use ordered structures, and invalidation emits a deletion cookie.

See :doc:`/topics/network/using-http-servers` for configuration, routing, body handling, streamed responses, TLS, and
cookie-session examples.

Interface
=========

.. doxygenenum:: erbsland::network::HttpCookieSecurePolicy
.. doxygenclass:: erbsland::network::HttpCookieSessionManager
    :members:

.. doxygentypedef:: erbsland::network::HttpCookieSessionManagerPtr
.. doxygenclass:: erbsland::network::HttpCookieSessionManagerOptions
    :members:
.. doxygenclass:: erbsland::network::HttpMediaTypeMapping
    :members:
.. doxygenclass:: erbsland::network::HttpServer
    :members:

.. doxygentypedef:: erbsland::network::HttpServerPtr
.. doxygenclass:: erbsland::network::HttpServerEventEditor
    :members:
.. doxygenclass:: erbsland::network::HttpServerOptions
    :members:
.. doxygenclass:: erbsland::network::HttpServerRequest
    :members:

.. doxygentypedef:: erbsland::network::HttpServerRequestPtr
.. doxygenclass:: erbsland::network::HttpServerRequestEventEditor
    :members:
.. doxygentypedef:: erbsland::network::HttpServerRequestFn

.. doxygentypedef:: erbsland::network::HttpServerTextRequestFn

.. doxygentypedef:: erbsland::network::HttpServerJsonRequestFn

.. doxygentypedef:: erbsland::network::HttpServerRequestHeadFn

.. doxygentypedef:: erbsland::network::HttpServerSessionFn
.. doxygenclass:: erbsland::network::HttpServerRouteOptions
    :members:
.. doxygenclass:: erbsland::network::HttpServerSession
    :members:

.. doxygentypedef:: erbsland::network::HttpServerSessionPtr

.. doxygentypedef:: erbsland::network::HttpServerSessionWeakPtr

.. doxygentypedef:: erbsland::network::HttpSessionDataPtr
.. doxygenclass:: erbsland::network::HttpServerSessionContext
    :members:
.. doxygenclass:: erbsland::network::HttpServerSessionEventEditor
    :members:
.. doxygenclass:: erbsland::network::HttpServerSessionManager
    :members:

.. doxygentypedef:: erbsland::network::HttpServerSessionManagerPtr
.. doxygenclass:: erbsland::network::HttpServerSessionSelection
    :members:
.. doxygenclass:: erbsland::network::HttpServerTlsOptions
    :members:
.. doxygenclass:: erbsland::network::HttpSessionData
    :members:
.. doxygenclass:: erbsland::network::HttpStaticContent
    :members:
.. doxygenclass:: erbsland::network::HttpStaticContentHandler
    :members:
.. doxygenclass:: erbsland::network::HttpStaticFileHandler
    :members:
.. doxygenclass:: erbsland::network::HttpStaticResourceHandler
    :members:
