.. index::
    single: HTTP Server; Using
    single: HTTP Routing
    single: HTTP Request Body
    single: Cookie Session

******************
Using HTTP Servers
******************

An ``HttpServer`` keeps the transport and HTTP/1.1 machinery behind a session-first interface.
This page shows how to bind a server, add ordered routes, work with request bodies and delayed responses, stream output
under back-pressure, enable TLS, and share logical sessions through secure cookies.

Starting a Plaintext Server
===========================

Create and configure the server on its owner event loop.
Route registration is additive, so every ``onRequest`` call adds another candidate instead of replacing an earlier one:

.. code-block:: cpp

    auto server = events->get<network::Network>().createHttpServer();
    server->events()
        .onRequest(
            network::HttpMethod{network::HttpMethodType::Get},
            "/health"_el,
            [](network::HttpServerSessionPtr, network::HttpServerRequestPtr request, mem::ByteBlock) {
                request->sendJson("{\"status\":\"ok\"}"_el);
            })
        .onRequest(
            network::HttpMethod{network::HttpMethodType::Get},
            "/users/{id}"_el,
            [](network::HttpServerSessionPtr, network::HttpServerRequestPtr request, mem::ByteBlock) {
                request->sendText(*request->parameter("id"_el));
            })
        .onError(reportNetworkError)
        .onFinal([]() {
            applicationStopped();
        });

    server->start(network::IpEndpoint{
        network::IpAddress::loopbackV4(), network::Port{8080U}});

Pass an automatic port to bind an ephemeral listener.
After ``onListening`` runs, ``localEndpoint()`` contains the effective address and port.
Call ``pauseAccepting()`` and ``resumeAccepting()`` for temporary admission control.
``close()`` performs an orderly shutdown, while ``abort()`` stops immediately.

How Routes Match
================

The first matching registration wins.
Session-specific routes are searched first, then server routes.
This makes it possible to install an authenticated or state-dependent route when the session is created:

.. code-block:: cpp

    server->events().onNewSession([](network::HttpServerSessionPtr session) {
        session->events().onRequest(
            network::HttpMethod{network::HttpMethodType::Get},
            "/account"_el,
            [](network::HttpServerSessionPtr session, network::HttpServerRequestPtr request, mem::ByteBlock) {
                request->sendJson(renderAccount(session->data()));
            });
    });

Patterns inspect only the decoded path.
The query remains available unchanged through ``query()``.
A ``{name}`` parameter occupies one complete segment; a final ``{*path}`` parameter captures the remaining segments.
The server splits on literal slashes before percent-decoding, so an encoded slash stays inside its original parameter.
Malformed escapes and invalid UTF-8 produce a bounded bad-request response.

Register an explicit ``HEAD`` handler when it differs from ``GET``.
Otherwise the matching ``GET`` handler runs and the framework suppresses its response body bytes while retaining the
correct fixed-response length.

Serving Static Content
======================

Register static content before starting the server.
Filesystem and resource handlers share prefix, priority, index, and media-type configuration:

.. code-block:: cpp

    auto mediaTypes = network::HttpMediaTypeMapping::defaultMapping()->copy();
    mediaTypes->setSuffix(".manifest"_el, "application/manifest+json"_el);

    auto files = network::HttpStaticFileHandler::create(path::Path{"web"_el}, "/assets"_el);
    files->setMediaTypeMapping(mediaTypes);

    auto resources = network::HttpStaticResourceHandler::create("application-ui"_el, "/"_el);
    resources->setPriority(-10).setMediaTypeMapping(mediaTypes);

    server->addStaticContentHandler(files);
    server->addStaticContentHandler(resources);

The resource factory above borrows ``Application::resources()``.
Pass a ``ResourcesConstPtr`` as the first argument to retain an injected provider, which is useful for tests and custom
stores.
Providers must support concurrent calls to their const lookup methods.

Dynamic session and server routes always win.
Static handlers are searched by descending priority and then registration order; a missing candidate falls through to
the next handler.
This makes overlays explicit without granting filesystem roots access to one another.
Prefixes and resource paths use the request target's already-decoded NFC segments, and resource lookup remains exact and
case-sensitive.
The first positive ``hasPath()`` probe owns the request; later content failures are server errors rather than overlay
fall-through.

``GET`` streams bounded blocks under connection back-pressure and ``HEAD`` returns the same metadata without loading a
resource body or reading file blocks.
Existing content requested with another method returns ``405``.
Only URLs ending in ``/`` try index names, which default to ``index.html`` and ``index.htm``.
Non-slash paths are exact and never infer directories or redirect.
Links, special files, unsafe path spellings, directory listings, ranges, validators, compression negotiation, and
caching are deliberately outside this increment.

For a custom source, derive from ``HttpStaticContentHandler`` and implement thread-safe ``hasPath()`` and
``getContent()`` methods.
Return an ``HttpStaticContent`` that reports an exact finite length and opens a non-null ``ByteInputStream`` once.
The server invokes probes, content creation, opening, and reads on workers, while all application callbacks stay on the
owner loop.
Handler and mapping setters throw while any using server is starting or active and become available again after all such
servers reach a terminal state.

Reading a Request Body
======================

``onRequest`` aggregates bytes before invoking the handler, with a one-MiB default limit.
``onTextRequest`` additionally validates strict UTF-8, while ``onJsonRequest`` parses one complete JSON value:

.. code-block:: cpp

    server->events().onJsonRequest(
        network::HttpMethod{network::HttpMethodType::Post},
        "/messages"_el,
        [](network::HttpServerSessionPtr,
           network::HttpServerRequestPtr request,
           text::json::JsonValue body) {
            storeMessage(body);
            request->sendJson(makeAcknowledgement(body), network::HttpStatus::Accepted);
        },
        network::HttpServerRouteOptions{}.setMaximumBodyLength(unit::ByteLength{64U * 1024U}));

Automatic routes can restrict accepted representation types with exact patterns, ``type/*``, ``*/*``, or structured
suffix patterns such as ``application/*+json``.
Text routes accept ``text/*`` by default and JSON routes accept ``application/json`` and ``application/*+json``; both
reject non-UTF-8 charsets.
An explicitly empty accepted-type list disables filtering.

Use ``onRequestHead`` and ``streamBody()`` with ``onBodyData`` for larger input.

.. code-block:: cpp

    server->events().onRequestHead(
        network::HttpMethod{network::HttpMethodType::Post},
        "/upload"_el,
        [](network::HttpServerSessionPtr, network::HttpServerRequestPtr request) {
            request->events()
                .onBodyData(consumeUploadBlock)
                .onBodyCompleted([request]() {
                    request->sendText("stored"_el, network::HttpStatus::Created);
                });
            request->streamBody();
        });

``pauseBody()`` propagates back-pressure to the connection; ``resumeBody()`` continues delivery.
``onTrailers`` reports decoded trailers, and ``onBodyCompleted`` marks the end of the streamed body.
``rejectBody()`` keeps the response side available but prevents connection reuse.

If a handler starts a response before selecting a body policy, the framework automatically rejects unread input.
A retained request can complete later from another event posted to the same owner loop.
If its deadline has already ended the transaction, fixed and framing reply operations are safe no-ops and cannot affect
a later request on the same connection.

Streaming a Response
====================

Fixed helpers such as ``sendText``, ``sendJson``, ``sendError``, and ``sendRedirect`` commit one bounded response.
For generated or large output, start a chunked response and submit complete blocks atomically:

.. code-block:: cpp

    auto headers = network::HttpHeaders{};
    headers.setField(network::HttpFieldType::TransferEncoding, "chunked"_el);
    auto head = network::HttpResponseHead{
        network::HttpVersion::Http11,
        network::HttpStatus::Ok,
        "OK"_el,
        std::move(headers)};

    request->startResponse(std::move(head));
    pumpNextBlock(request);

When ``sendBody()`` reports ``WouldBlock``, keep that block and retry it from ``onWritable``.
Finish the stream with ``finishBody()``, optionally passing trailer fields; the framework retries finishing internally.
Never submit a different block while an earlier one is waiting for capacity.

Enabling HTTPS
==============

Register the server identity under ``http/server`` (or a parent/global fallback) in the application cryptology
configuration as described in :doc:`using-tls-server-connections`.
Then enable HTTPS before starting:

.. code-block:: cpp

    server->enableTls();
    server->start(network::IpEndpoint{network::IpAddress::anyV6(), network::Port{8443U}});

Use ``HttpServerTlsOptions`` only when the identity label, exact SNI mappings, handshake capacity, or handshake timeout
must differ from the secure defaults:

.. code-block:: cpp

    auto tls = network::HttpServerTlsOptions{};
    tls.setMaximumConcurrentHandshakes(unit::ItemCount{64U});
    server->enableTls(std::move(tls));

The same routes and request types handle plaintext and authenticated TLS streams.
``connection()`` exposes the concrete ``ConnectionPtr`` when transport metadata is needed.
The HTTP server owns cipher, TLS buffer, accepted TCP, and graceful-close integration and always forces the single
``http/1.1`` ALPN identifier.
Connections that do not negotiate it are rejected.

Complete HTTPS Server Demo
==========================

The ``https_server`` demo combines the pieces above in one standalone application.
It loads a PEM certificate chain and matching PKCS#8 key, registers fixed and parameterized routes, streams generated
output with writable-event retry, and mounts repeated filesystem roots as one ordered ``/assets`` overlay.
The first ``--static-root`` has the highest priority.

.. erbsland-demo::
    :source: network/HttpsServer/HttpsServerApp.cpp
    :source-sha256: 41304731977290ced788783c0ff4f82881bab6fec3e1b29203db2915b1941c6c

.. code-block:: cpp

    /// Register the certificate and matching private key under the HTTP server's standard TLS configuration label.
    void HttpsServerApp::configureTls() {
        auto configuration = el::cryptology::TlsConfiguration{};
        configuration.setServerIdentity(
            el::cryptology::TlsServerIdentity{
                el::cryptology::X509CertificateBundle::fromFileOrThrow(_certificatePath),
                el::cryptology::SigningPrivateKey::fromPemOrThrow(_privateKeyPath.content().readTextOrThrow())});
        cryptologyConfiguration().setTlsConfiguration("http/server"_el, std::move(configuration));
    }

    /// Mount repeated static roots at one URL prefix, preserving command-line order as overlay priority.
    void HttpsServerApp::configureStaticContent() {
        auto priority = std::int32_t{static_cast<std::int32_t>(_staticRoots.size())};
        for (const auto &root : _staticRoots) {
            auto handler = el::HttpStaticFileHandler::create(root, "/assets"_el);
            handler->setPriority(priority--);
            _server->addStaticContentHandler(std::move(handler));
        }
    }

    /// Register fixed, parameterized, and streamed routes through the HTTP server event editor.
    void HttpsServerApp::configureRoutes() {
        _server->events()
            .onRequest(
                el::HttpMethod{el::HttpMethodType::Get},
                "/health"_el,
                [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) -> void {
                    request->sendJson("{\"status\":\"ok\"}"_el);
                })
            .onRequest(
                el::HttpMethod{el::HttpMethodType::Get},
                "/hello/{name}"_el,
                [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) -> void {
                    request->sendText(
                        el::StringFormat{"Hello, {}!"_el}.build(request->parameter("name"_el).value_or("visitor"_el)));
                })
            .onRequestHead(
                el::HttpMethod{el::HttpMethodType::Get},
                "/stream"_el,
                [this](el::HttpServerSessionPtr, el::HttpServerRequestPtr request) -> void {
                    auto response = std::make_shared<StreamedResponse>(
                        std::move(request), [this](StreamedResponse *finished) -> void { removeResponse(finished); });
                    _responses.emplace_back(response);
                    response->start();
                });
    }

    /// Create the HTTPS server, attach lifecycle callbacks, and bind its configured endpoint.
    void HttpsServerApp::startServer() {
        _server = events()->get<el::Network>().createHttpServer();
        _server->enableTls();
        configureStaticContent();
        configureRoutes();
        _server->events()
            .onListening([this]() -> void { onListening(); })
            .onClosed([]() -> void { el::stdOut()->printLine("HTTPS server closed gracefully."_el); })
            .onError([](const el::NetworkErrorContext &error) -> void { throw el::network::NetworkError{error}; })
            .onFinal([this]() -> void { quit(); });
        _server->start(el::IpEndpoint{_address, _port});
    }

    void HttpsServerApp::onListening() {
        el::stdOut()->printLine("HTTPS server listening on "_el, _server->localEndpoint()->toString());
        el::stdOut()->printLine("Routes: /health, /hello/{name}, /stream, /assets/..."_el);
        if (_runFor.isPositive()) {
            events()->invokeAfter(_runFor, [this]() -> void { closeGracefully(); });
        }
    }

    /// Stop accepting new connections and let active HTTPS responses drain before quitting the application.
    void HttpsServerApp::closeGracefully() {
        if (_closing) {
            return;
        }
        _closing = true;
        el::stdOut()->printLine("Initiating graceful shutdown..."_el);
        _server->close();
    }

    void HttpsServerApp::removeResponse(StreamedResponse *finished) {
        events()->invoke([this, finished]() -> void {
            std::erase_if(_responses, [finished](const auto &response) -> bool { return response.get() == finished; });
        });
    }

.. erbsland-demo-end::

The streamed-response helper retains a block when the atomic send reports back-pressure and retries that exact block
from ``onWritable``:

.. erbsland-demo::
    :source: network/HttpsServer/StreamedResponse.cpp
    :source-sha256: 4910bdf3eedab266ab288fe07c54927db82b5c08086ba5e83db6292ea0c894d9

.. code-block:: cpp

    /// Start a chunked response and register callbacks that continue or release the response at lifecycle checkpoints.
    void StreamedResponse::start() {
        const auto weakSelf = weak_from_this();
        _request->events()
            .onWritable([weakSelf]() -> void {
                if (const auto self = weakSelf.lock()) {
                    self->pump();
                }
            })
            .onFinal([weakSelf]() -> void {
                if (const auto self = weakSelf.lock()) {
                    self->_done(self.get());
                }
            });

        auto headers = el::HttpHeaders{};
        headers.setField(el::HttpFieldType::TransferEncoding, "chunked"_el);
        headers.setContentType(el::HttpMediaType::fromStringOrThrow("text/plain; charset=utf-8"_el));
        _request->startResponse(
            el::HttpResponseHead{el::HttpVersion::Http11, el::HttpStatus::Ok, "OK"_el, std::move(headers)});
        pump();
    }

    /// Keep one rejected block unchanged and retry it only after the request reports writable capacity.
    void StreamedResponse::pump() {
        while (!_finished && _lineIndex < cLineCount) {
            if (_pendingBlock.isEmpty()) {
                const auto line =
                    el::StringFormat{"generated line {:03d}: event-driven HTTPS output\n"_el}.build(_lineIndex + 1U);
                _pendingBlock = el::StringEncoder{line}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);
            }
            const auto status = _request->sendBody(_pendingBlock);
            if (status.wouldBlock()) {
                return;
            }
            if (status.isClosed()) {
                _finished = true;
                return;
            }
            _pendingBlock = {};
            ++_lineIndex;
        }
        if (!_finished) {
            _finished = true;
            _request->finishBody();
        }
    }

.. erbsland-demo-end::

Start it with one or more static roots.
The optional duration demonstrates graceful shutdown by stopping admission and draining active responses before the
application exits:

.. code-block:: console

    $ https_server --certificate server.pem --private-key server-key.pem \
          --static-root ./site-overlay --static-root ./site-base --run-for 60

The server exposes ``/health``, ``/hello/{name}``, ``/stream``, and ``/assets/...``.
Use ``--port 0`` to select an ephemeral port; the effective endpoint is printed after binding.

Using Cookie-Backed Sessions
============================

The default manager creates one anonymous session per connection.
Install ``HttpCookieSessionManager`` when the same logical session must continue across connections:

.. code-block:: cpp

    auto cookieOptions = network::HttpCookieSessionManagerOptions{};
    cookieOptions
        .setCookieName("service-session"_el)
        .setCookiePath("/api"_el)
        .setIdleTimeout(time::TimeDelta::minutes(20))
        .setAbsoluteTimeout(time::TimeDelta::hours(8))
        .setMaximumSessions(unit::ItemCount{5000U});
    server->setSessionManager(
        network::HttpCookieSessionManager::create(std::move(cookieOptions)));

The manager stores only a secure-random identifier in the cookie and keeps session data in a bounded server-side
registry.
Unknown identifiers are replaced instead of adopted, which prevents session fixation.
Cookies are host-only, ``HttpOnly``, and ``SameSite=Lax`` by default; HTTPS adds ``Secure`` automatically.
Idle and absolute expiry are enforced by ordered owner-loop cleanup, with least-recently-used eviction at capacity.
Calling ``invalidate()`` removes the registry entry and adds a deletion cookie to the current response.

The manager owns its reserved cookie name.
Application responses that attempt to set the same cookie are rejected so session state cannot be replaced accidentally.
