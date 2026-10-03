..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Communication Between Loops
    single: Events; Request and Reply
    single: Events; Obsolete Replies

*********************************
Communicating Between Event Loops
*********************************

A worker can calculate a result while your main loop continues responding to input.
The useful boundary is ownership: each component updates its state on its own loop, and sends other components commands
or results that own their data.
This page develops a request and reply exchange, then handles input that replaces a request before its reply arrives.

This example builds on worker creation in :doc:`processing_work_on_event_threads`, recipient lifetime in
:doc:`callback_lifetimes`, and the return path introduced in :doc:`event_bindings`.
Here the main loop stays running throughout the exchange; it is the receiver as well as the producer of requests.

State Belongs to a Loop
=======================

Choose one loop to own each component's mutable state.
A preview controller, for example, can own the selected input, latest request identifier, and displayed result on main.
A worker receives a copy of the input and produces a separate result.
The worker never reads the controller's changing selection and never writes its displayed result directly.

:cpp:func:`Events::invoke() <erbsland::event::Events::invoke>` queues a callback on the receiving loop.
Calling an ordinary method does not move execution to another thread, even when the object owns an event target.
To call a worker-owned component, capture its independent input and queue the method call through that component's
:cpp:class:`Events <erbsland::event::Events>` target, retained as an ``EventsPtr``.
Only the callback executing there accesses its mutable state.

.. mermaid::

    sequenceDiagram
        participant Main as Main loop / preview state
        participant Worker as Worker loop / calculation
        Main->>Worker: invoke(command with input and bound reply)
        Note over Main: Return to dispatch, other input can run
        Worker->>Worker: Calculate from owned input
        Worker->>Main: Call bound reply with result
        Main->>Main: Check identifier and update preview

This arrangement needs no mutex around preview state because one thread performs all its reads and writes.
Capturing a shared pointer does not provide the same guarantee: if two loops mutate its pointee, you still need
synchronization or a different ownership design.
Likewise, references and views into the sender's mutable buffer are not independent input values.

A Running Main as the Reply Target
==================================

The text-preview application transforms text on one managed worker and receives the result on main.
Uppercasing a short string makes the exchange easy to inspect; a real application could put an expensive parser or image
conversion at the same worker boundary.
The sample does not need another thread for the cost of this particular transformation.

The application object is the controller and outlives the exchange and worker shutdown.
Its retained members make ownership explicit:

.. erbsland-demo::
    :source: event/TextPreview/TextPreviewApp.hpp
    :sha256: 21d2083486ee62d3b4fc1e493b4f7fb990c8bda78b4a33c59dbf7205827176ed

.. code-block:: cpp

    /// Keep preview state on main while a worker returns independently owned text values.
    class TextPreviewApp final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application
        void initialize() override;
        void registerCommandLineOptions(const el::OptionsPtr &options) override;
        [[nodiscard]] auto main() -> el::ExitCode override;
        void cleanup() noexcept override;

    private:
        void begin();
        void requestPreview(el::String text);
        void receivePreview(std::uint64_t requestId, el::String preview);

    private:
        el::ManagedEventThreadPtr _worker; ///< Retained on main until application shutdown joins it.
        el::EventTimerPtr _deadline;       ///< Main-loop timer bounds the wait for replies.
        el::String _scenario;              ///< Configured before dispatch begins; read only on main.
        std::uint64_t _requestId{0};       ///< Latest request identity, owned by main.
        int _received{0};                  ///< All replies, including obsolete ones, owned by main.
        int _accepted{0};                  ///< Replies allowed to update the preview, owned by main.
        el::String _preview;               ///< Display state, owned by main.
    };

.. erbsland-demo-end::

The derived ``main()`` starts the worker, posts ``begin()`` to main, and enters
:cpp:func:`runEventLoop() <erbsland::core::Application::runEventLoop>`.
It checks completion after dispatch ends.
The worker uses :cpp:enumerator:`FailApplication <erbsland::event::EventLoopErrorPolicy::FailApplication>` so an
exception cannot silently remove a required reply; see the failure path below.
The worker handle stays in ``_worker`` until application shutdown has joined it.

For one request, ``begin()`` calls ``requestPreview()`` from a callback on main.
That context matters: :cpp:func:`currentEvents() <erbsland::event::currentEvents>` identifies the loop currently
executing on the calling thread.
Here ``bindEvent`` captures that reply address before dispatch crosses to the worker:

.. erbsland-demo::
    :source: event/TextPreview/TextPreviewApp.cpp
    :show: f:TextPreviewApp::requestPreview
    :exec: event/text_preview --scenario single
    :show-cmd-line:
    :sha256: 7ea6044f06cb1dc53ae44da1a595ef237a8f0d3cb8532394eb2815f446f591c4

.. code-block:: cpp

    void TextPreviewApp::requestPreview(el::String text) {
        const auto requestId = ++_requestId;
        // Bind the reply on main before sending work to another loop.
        auto reply =
            el::bindEvent([this, requestId](el::String preview) -> void { receivePreview(requestId, std::move(preview)); });
        _worker->events()->invoke([reply = std::move(reply), text = std::move(text)]() -> void {
            // Worker uses only captured input; main owns every application-state update.
            reply(el::toUppercase(text));
        });
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/text_preview --scenario single

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAccepted preview: FIRST DRAFT␛[0m

.. erbsland-demo-end::

The worker callback owns the input and the bound reply.
:cpp:func:`toUppercase() <erbsland::text::toUppercase>` derives a new string value without changing the captured input.
Calling ``reply`` transfers the computed preview into a delivery for main.
The binding's handler retains the request identifier, so ``receivePreview()`` can associate that value with its input.
Only dispatch on main dereferences ``this`` to update controller state.

Creating the binding inside the worker callback would capture the worker loop instead.
Create it at the sending end, or pass the intended target explicitly when creating it outside a running loop.
The function cannot infer which component should receive a result.

Target Lifetime and Recipient Lifetime
======================================

The bound reply retains its event target.
It does not retain the controller addressed by ``this``, keep its thread running, or promise continued dispatch.
The application's lifetime supplies the recipient guarantee here: the stack-owned application survives ``run()``,
including managed-thread shutdown, while worker callbacks can refer to it.

For a shorter-lived controller owned by a shared pointer, a member binding can retain it strongly, or ``bindWeakEvent``
can suppress delivery after it disappears.
A weak reply also needs a completion policy: another component waiting for the result must be able to cancel its wait or
observe a deadline.
:doc:`event_bindings` demonstrates optional weak delivery separately from required operation completion.

Replacing a Request Before Its Reply
====================================

Suppose the user edits the input while conversion is pending.
A reply can be valid for its original request and still be wrong for the currently selected input.
Assign an identifier on the controller's loop and return it unchanged with the result.
The reply may update the preview only if its identifier is still current.

The replacement scenario submits both inputs in the same main-loop callback:

.. erbsland-demo::
    :source: event/TextPreview/TextPreviewApp.cpp
    :show: c:Submit initial input(+0,+4)
    :sha256: b2e17703bfbf701634d13356ef2a611f3de53558d59a8972c828cc61ea18c8d4

.. code-block:: cpp

    // Submit initial input, then optionally replace it before main can dispatch a reply.
    requestPreview("First draft"_el);
    if (_scenario == "replacement"_el) {
        requestPreview("Revised draft"_el);
    }

.. erbsland-demo-end::

Even if the worker finishes the first input immediately, main cannot dispatch that reply until ``begin()`` returns.
By then ``_requestId`` identifies the revised input.
This creates an obsolete reply deterministically, without sleeps or assumptions about relative thread speed.
The old result arrives after replacement, even though this single worker returns the two results in submission order.

.. erbsland-demo::
    :source: event/TextPreview/TextPreviewApp.cpp
    :show: f:TextPreviewApp::receivePreview
    :exec: event/text_preview --scenario replacement
    :show-cmd-line:
    :sha256: 111b503a2fa5d85e162d216306185d9f92466aa58be212c65782efdc174ac82d

.. code-block:: cpp

    void TextPreviewApp::receivePreview(const std::uint64_t requestId, el::String preview) {
        if (el::currentEvents() != events()) {
            throw el::LogicFailure{"Preview delivered to the wrong loop."_el};
        }
        ++_received;
        if (requestId == _requestId) {
            _preview = std::move(preview);
            ++_accepted;
            el::io::printLine("Accepted preview: "_el, _preview);
        } else {
            el::io::printLine("Ignored obsolete preview."_el);
        }
        if (_received == static_cast<int>(_requestId)) {
            _deadline->stop();
            quit();
        }
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/text_preview --scenario replacement

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mIgnored obsolete preview.
        Accepted preview: REVISED DRAFT␛[0m

.. erbsland-demo-end::

``_preview`` changes only for the current identifier.
``_received`` counts both replies because this bounded demo verifies the complete exchange before quitting.
An obsolete reply is ignored for display but remains part of the demo's completion accounting.
The post-loop checks require both expected replies, one accepted result, and the expected preview text.

Waiting for every obsolete reply is a demonstration choice, not a requirement for an interactive controller.
A real controller can retire old request state as soon as its protocol permits, and keep running for further input.
For success, cancellation, and timeout races within one operation, also check whether that operation is still pending.
A matching identity alone does not establish that a result is still needed.

The identity check remains useful with multiple workers, where results can arrive in a different order from requests.
Independent loops and competing producers do not establish a global execution order.
If operations must run in a particular sequence, encode the dependency in messages or keep them on one owner loop.

Completion, Failure, and Responsiveness
=======================================

The sample retains a one-second timer on main while replies are outstanding.
A missing reply therefore ends the exchange with an error instead of leaving the application running indefinitely.
When all replies arrive, it stops that timer and requests application shutdown.
The ``missing`` scenario deliberately submits no work after arming the deadline, verifying that the wait fails.
The ``failure`` scenario queues a throwing worker callback, verifying managed failure propagation.
Both exit with failure; ``single`` and ``replacement`` verify their expected results and exit successfully.

The main loop's error handler ends dispatch on a callback exception, and the code after ``runEventLoop()`` retrieves and
rethrows the stored error for application diagnostics.
On the worker, ``FailApplication`` reports the failure to the application and starts managed shutdown.
Neither path assumes that continuing dispatch means a request succeeded.
The exact policy and propagation contracts are in :ref:`event-loop-options`.

Moving the conversion to a worker keeps main available for input and replies.
The worker loop still executes one callback at a time: a long conversion delays its later commands, including queued
cancellation requests.
A deadline on main can detect missing progress, but cannot interrupt that conversion or make its eventual join finish.
For cooperative cancellation and teardown ordering, continue with :doc:`shutting_down_event_work`.

Main remains responsive because it returns to dispatch after submitting each request.
Blocking there to wait for a reply would prevent the reply callback from running on that same loop.
The procedural file hasher in :doc:`processing_work_on_event_threads` can wait because a separate, running receiver owns
its entire completion path.
If the worker needs dependencies, readiness reporting, or asynchronous resource closure, the same message flow can
connect application parts; :doc:`using_event_loops_in_application_parts` explains that lifecycle.
