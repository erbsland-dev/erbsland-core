..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********************************************
FileServer: A Service in an Application Part
********************************************

``FileServer`` demonstrates a long-lived service owned by an
:cpp:class:`ApplicationPart <erbsland::core::ApplicationPart>`.
The part creates an HTTP server on its dedicated loop, serves a directory on loopback, and keeps dispatch available
while committed responses drain during shutdown.
The directory listing is supporting functionality; the main lessons are part lifecycle, source ownership, and graceful
completion.
See :doc:`/topics/event/using_event_loops_in_application_parts` for the general lifecycle.

Registration and Configuration
==============================

The entry point registers ``ServerPart`` before ``run()`` and enables service lifecycle support.
It leaves the default application main in place, so the framework starts the registered part and runs the main loop.
No manual thread creation is necessary.

.. erbsland-demo::
    :source: event/apps/FileServer/main.cpp
    :show: c:Register an application part(+0,+15)
    :sha256: d63d1dbb49cb0c9428134659e55c2e317d33e0ab93759fe48090575954a49321

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

The part's command-line hooks register and validate the root directory, listening port, and optional timeout before the
part starts.
Port zero asks the system to choose an available port.
The root is resolved physically and must be a directory; negative timeouts are rejected.
These hooks retain configuration for later use by the part's dedicated loop.

The part also implements a public ``ServerStatus`` interface through ``ApplicationPartWithInterface<ServerStatus>``.
The retained state subscription runs on the application's main loop and reads a request-count snapshot when the server
part enters ``Stopping``.
The getter shares a mutex with the directory-index request callback, so the read is safe while that callback is still
active on the part thread.
Only requests for the directory index are counted; static file downloads are not included.
The printed value is a snapshot at shutdown initiation, not a guarantee that all requests have finished.

A Source Bound to the Part Loop
===============================

:cpp:func:`ApplicationPart::onInitialize() <erbsland::core::ApplicationPart::onInitialize>` already runs inside the part's loop.
It uses ``loop()->get<el::Network>()`` to create the HTTP source and retains it in ``_server``.
A static content handler serves files below ``/files``; a separate GET handler builds the root listing.
The listing walks regular files, skips symbolic links, and limits the displayed entries to 1000.

The editor connects readiness, failure, and final completion before the server starts.
``onListening()`` is the useful service-readiness notification: creating the source or returning from part
initialization alone does not mean the endpoint is ready.
Only after listening succeeds does a positive demo timeout schedule application quit.

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: f:onInitialize
    :sha256: 95475d526f01d47e4e03d0c58bfbb0075616fe2c66510e2d540f9dc901077efe

.. code-block:: cpp

    void onInitialize() override {
        // The network source belongs to this part's event loop.
        _server = loop()->get<el::Network>().createHttpServer();
        _server->addStaticContentHandler(el::HttpStaticFileHandler::create(_root, "/files"_el));
        // Handle directory-index requests on the part thread.
        _server->events().onRequest(
            el::HttpMethod{el::HttpMethodType::Get},
            "/"_el,
            [this](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) -> void {
                recordIndexRequest();
                auto headers = el::HttpHeaders{};
                headers.setField(el::HttpFieldType::ContentType, "text/html; charset=utf-8"_el);
                const auto body = createIndex();
                request->sendResponse(
                    el::HttpResponseHead{el::HttpVersion::Http11, el::HttpStatus::Ok, "OK"_el, headers},
                    el::StringEncoder{body}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject));
            });
        // Report listener readiness after the endpoint has been bound.
        _server->events().onListening([this]() -> void {
            el::io::printLine("Browse http://"_el, _server->localEndpoint()->toString(), "/"_el);
            if (_timeout.isPositive()) {
                loop()->invokeAfter(_timeout, []() -> void { el::application().quit(); });
            }
        });
        // Escalate source errors to the part manager.
        _server->events().onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; });
        // Complete part shutdown after the source has closed.
        _server->events().onFinal([this]() -> void {
            if (state() == el::ApplicationPartState::Stopping) {
                completeStopping();
            }
        });
        // Bind to loopback so the demo exposes files only on the local machine.
        _server->start(el::IpEndpoint{el::IpAddress::fromStringOrThrow("127.0.0.1"_el), _port});
    }

.. erbsland-demo-end::

Requests and source notifications execute on the part loop, separately from the application main loop.
The server binds to ``127.0.0.1`` so this example exposes the selected directory only on the local machine.
The ``onError()`` handler throws ``NetworkError`` to put operational failure on the application's managed failure path.

Closing Before Ending Dispatch
==============================

Application quit begins the part's stopping phase.
``onStopping()`` checks whether the source is already terminal; otherwise it calls ``close()`` and leaves completion
pending.
The part loop keeps dispatching while committed responses drain.
``onFinal()`` calls ``completeStopping()`` when the part is stopping, allowing the framework to finish its lifecycle.

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: f:onStopping, f:cleanup
    :exec: event/file_server {directory:served} --port 0 --timeout 1
    :show-cmd-line:
    :sha256: 1bb2825345253ad6a32545ae1c7dcdf65cb0a97230067c453506b3dec38876b5

.. code-block:: cpp

    void onStopping() override {
        // Allow committed responses to drain before ending this part's event loop.
        if (_server == nullptr || _server->state() == el::NetworkSourceState::Closed ||
            _server->state() == el::NetworkSourceState::Failed) {
            completeStopping();
        } else {
            _server->close();
        }
    }

    void cleanup() noexcept override {
        if (_server != nullptr) {
            _server->abort();
            _server.reset();
        }
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/file_server served --port 0 --timeout 1

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBrowse http://127.0.0.1:51641/
        Directory-index requests at shutdown: 0␛[0m

.. erbsland-demo-end::

The run above starts on an available port and exits after one second without requiring an HTTP client.
The displayed address varies by run.
To browse the service yourself, supply a directory and allow a longer timeout.

``cleanup()`` is the final fallback: it aborts any retained source and releases it, including after partial startup or
failure.
It does not wait for another event callback.
The distinction between graceful ``close()`` and fallback ``abort()`` is developed in
:doc:`/topics/event/shutting_down_event_work`.
