.. index::
    single: HTTP Client; Using
    single: HTTP Upload
    single: HTTP Download

******************
Using HTTP Clients
******************

An ``HttpClientSession`` gives related HTTP and HTTPS requests one place for defaults, limits, callbacks, and lifecycle.
This page shows how to submit requests, choose automatic or low-level response handling, stream bodies under
back-pressure, and configure authenticated HTTPS.

Sending a Request
=================

Create the session on its owner event loop.
It starts active, so you can install fallback callbacks and submit a request immediately:

.. code-block:: cpp

    auto session = events->get<network::Network>().createHttpClientSession();
    session->events()
        .onJsonResponse([](
            network::HttpClientRequestPtr request,
            network::HttpClientResponsePtr response,
            text::json::JsonValue body) {
            consumeResult(request->url(), response->head().status(), std::move(body));
        })
        .onError([](network::HttpClientRequestPtr request, const network::NetworkErrorContext &error) {
            reportFailure(request->url(), error);
        });

    auto headers = network::HttpHeaders{};
    headers.setField("Authorization"_el, "Bearer example"_el);
    session->setDefaultHeaders(std::move(headers));
    auto request = session->sendGet(network::Url::fromStringOrThrow("https://api.example.test/status"_el));

``sendGet()``, ``sendHead()``, and ``sendPost()`` return the retained request.
Use ``createRequest()`` when you need to set request-specific fields, choose its response callback, or prepare a
streamed upload before submission.
Session defaults are captured by ``sendRequest()``.
A request field replaces all same-name defaults, while repeated fields on the request retain their order.

The framework owns ``Host``, ``Content-Length``, and ``Transfer-Encoding``.
It generates an origin-form target from the absolute URL, excludes fragments, and preserves an explicitly empty query.
User information, ``CONNECT``, upgrades, and ``Expect`` are rejected.

Choosing Automatic Response Handling
====================================

Automatic handlers aggregate at most one MiB unless their options specify another bound.
The byte family accepts any representation.
The text family accepts an absent ``Content-Type`` or ``text/*``; the JSON family accepts an absent type,
``application/json``, or ``application/*+json``.
An explicit charset must be UTF-8. Supply accepted media-type patterns to replace these family rules, or an empty
pattern list to disable filtering.

A request handler overrides the corresponding session handler for that request:

.. code-block:: cpp

    auto request = session->createRequest(
        network::HttpMethod{network::HttpMethodType::Get},
        network::Url::fromStringOrThrow("https://api.example.test/readme"_el));
    request->events().onTextResponse([](
        network::HttpClientRequestPtr,
        network::HttpClientResponsePtr,
        text::String body) {
        showDocument(std::move(body));
    });
    session->sendRequest(request);

HTTP statuses, including 4xx and 5xx responses, reach the selected response handler.
The error callback is reserved for transport, TLS, HTTP framing, deadline, validation, resource, and sink failures.

Following Redirects
===================

Safe bounded redirects are enabled by default.
Configure the session before submission when an application needs a tighter boundary:

.. code-block:: cpp

    auto redirectOptions = network::HttpClientRedirectOptions{};
    redirectOptions
        .setMaximumRedirects(unit::ItemCount{5U})
        .setHostPolicy(network::HttpClientRedirectHostPolicy::SameRegistrableDomain);
    session->setRedirectOptions(redirectOptions);
    session->events().onRedirect([](
        network::HttpClientRequestPtr,
        network::HttpClientRedirectContext &context) {
        if (!context.canFollow()) {
            return network::HttpClientRedirectAction::ReturnResponse;
        }
        return network::HttpClientRedirectAction::Follow;
    });

The callback cannot make an unsafe target followable.
``ReturnResponse`` deliberately sends the 3xx response through the configured response handler; ``Reject`` reports an
operational error.
Request-specific redirect handlers and options override session fallbacks.
Inspect ``effectiveUrl()`` and ``redirectCount()`` on the final response while ``request->url()`` continues to identify
the submitted URL.

Using the Cookie Jar
====================

Automatic cookies are enabled by default and exclusively own the outgoing ``Cookie`` field.
Valid response cookies are stored before redirect handling, including cookies set by intermediate responses.
The stable session jar can be inspected and edited on its owner event loop:

.. code-block:: cpp

    auto &jar = session->cookieJar();
    auto insertion = network::HttpCookieInsertionOptions{};
    insertion.setPath("/api"_el).setSecure(true).setHttpOnly(true);
    jar.setCookie(
        network::Url::fromStringOrThrow("https://api.example.test/api"_el),
        "client-token"_el,
        "value"_el,
        insertion);
    for (const auto &cookie : jar.cookies()) {
        inspectCookie(cookie.name(), cookie.domain(), cookie.path(), cookie.expires());
    }

Invalid manual cookies fail synchronously; invalid peer cookies are ignored.
Disable automatic cookies in ``HttpClientSessionOptions`` before submitting a request if the application must provide a
raw ``Cookie`` field.

Streaming a Download
====================

Use ``onResponseHead`` when the body must not be aggregated automatically.
The callback receives the final head at a paused policy checkpoint.
Select exactly one policy for a body-bearing response:

.. code-block:: cpp

    request->events().onResponseHead([](
        network::HttpClientRequestPtr,
        network::HttpClientResponsePtr response) {
        const auto weakResponse = network::HttpClientResponseWeakPtr{response};
        response->events()
            .onBodyData([weakResponse](mem::ByteBlock data) {
                if (!consumeChunk(std::move(data))) {
                    if (const auto response = weakResponse.lock()) {
                        response->pauseBody();
                    }
                }
            })
            .onTrailers([](const network::HttpHeaders &trailers) {
                verifyTrailers(trailers);
            })
            .onBodyCompleted([]() {
                finishDownload();
            });
        response->streamBody();
    });

Call ``resumeBody()`` on the owner event loop after the consumer has capacity again.
For a blocking destination, ``writeBodyTo()`` performs one write of at most 16 KiB on a worker and pauses network input
until it completes.
Sink exceptions and failed writes are reported through the request error callback.

Pass a ``Path`` instead of a stream for an atomic download.
The client creates its temporary file in the destination directory and replaces an absent or regular-file destination
only after the response, writes, flush, and close all succeed:

.. code-block:: cpp

    request->events().onResponseHead([](
        network::HttpClientRequestPtr,
        network::HttpClientResponsePtr response) {
        response->events().onBodyProgress([](const network::HttpClientBodyProgress &progress) {
            updateProgress(progress.committedLength(), progress.expectedLength());
        });
        response->writeBodyTo(path::Path{"downloads/result.json"_el});
    });

Cancellation and failures remove the temporary file and preserve the previous destination.

Sequential Connection Reuse
===========================

The session automatically reuses completed HTTP/1.1 connections for sequential requests to the same origin.
It keeps at most one idle connection per origin and preserves logical request concurrency by opening another connection
when the retained one is leased.
Session options control the global idle count, idle deadline, transaction cap, and whether reuse is enabled.

Changes to session transport or TLS configuration invalidate incompatible idle connections.
A safe, replayable GET, HEAD, OPTIONS, or TRACE may be retried once when a leased idle connection proves stale before
any response is observed.
Other methods and streamed bodies are never retried automatically.

Streaming an Upload
===================

Select ``streamBody()`` while the request is prepared.
A send before the request is connected reports ``NetworkSendStatus::WouldBlock``.
``onWritable`` announces when upload can begin and when a rejected block can be retried:

.. code-block:: cpp

    request->streamBody();
    request->events().onWritable([request]() {
        const auto status = request->sendBody(nextBlock());
        if (status.isAccepted() && uploadComplete()) {
            static_cast<void>(request->finishBody());
        }
    });
    session->sendRequest(request);

Each call is atomic.
Retain a block after ``WouldBlock`` and retry only after ``onWritable``.
``finishBody()`` accepts optional trailers and follows the same atomic status contract.

HTTPS and Deadlines
===================

HTTPS uses the immutable TLS configuration selected by ``HttpClientTlsOptions``.
The default label is ``http/client`` and the session forces HTTP/1.1 ALPN.
Register explicit trust anchors for this label before sending HTTPS requests.

``HttpClientSessionOptions`` separates DNS, TCP connection, TLS handshake, response-head, body-idle, overall, and
graceful-close limits.
The overall deadline starts when the request is submitted, so time spent in the bounded pending queue counts toward it.
Diagnostic phases preserve lower-level resolving, connecting, handshaking, and closing contexts.

Closing the Session
===================

``close()`` rejects new submissions and drains queued and active requests.
It emits ``onClosed`` before the exactly-once ``onFinal`` callback.
``abort()`` cancels all submitted work immediately.
Individual requests can be cancelled without affecting their siblings.
