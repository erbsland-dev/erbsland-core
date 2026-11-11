..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Event Loops; Application Designs
    single: Events; Main Loop
    single: Events; Application Parts
    single: Application; Event Startup

***************************************
Running an Application with Event Loops
***************************************

An event-driven program needs both a place to start its work and a loop that dispatches its callbacks.
:cpp:class:`Application <erbsland::core::Application>` provides a lifecycle that connects these two responsibilities.
This page explains where to attach your startup code, how it reaches the intended loop, and how the application
finishes.
You can use the main loop directly, add lifecycle overrides, run workers from procedural code, or register an
application part with its own loop.

If you are still deciding how to organize your program, :doc:`/topics/core/choosing_an_application_design` compares the
available designs.
You can also start here: the application concepts needed for event work are introduced below.
For the underlying callback model, see :doc:`events_in_erbsland_core`.

How Application Runs Your Program
=================================

There are three different entry points to distinguish.
The global C++ ``main(argc, argv)`` creates one application object and returns ``app.run()``.
``Application::run()`` owns the surrounding lifecycle: initialization, command-line processing, application work, error
handling, and cleanup.
The virtual ``Application::main()`` is the work phase called by that lifecycle after successful command-line handling.
Calling ``run()`` therefore does considerably more than calling an application main function.

The default ``Application::main()`` starts automatically enabled application parts, if any, and enters the application's
main event loop on the calling main thread.
An event-driven program describes its work through callbacks.
The loop supplies the waiting and dispatch that let those callbacks make progress, including when there is temporarily
nothing ready to execute.
An empty queue does not mean the application has finished: the loop waits for more work until shutdown is requested.

This default makes the usual startup hooks sufficient for an event-driven program.
You prepare state in ``initialize()`` or validate parsed configuration in ``handleCommandLineOptions()``, then arrange
for work to begin when dispatch starts.
The following lifecycle shows where these hooks belong; its main-dispatch box uses the default event-loop behavior:

.. mermaid::

    flowchart TD
        entry["C++ main: create Application, call run()"] --> initialize["initialize()"]
        initialize --> options["Register and parse command-line options"]
        options --> result{"Parsing outcome"}
        result -->|"Successful values"| handle["handleCommandLineOptions(values)"]
        handle --> main["main(): start automatic parts, run main loop"]
        main -->|"Work finishes or fails"| shutdown["Stop parts and managed event threads"]
        result -->|"Help or version: skip main"| shutdown
        result -->|"Parsing failure: skip main"| shutdown
        initialize -.->|"Startup failure"| shutdown
        handle -.->|"Validation failure"| shutdown
        shutdown --> cleanup["cleanup()"]
        cleanup --> done["Report runtime errors if any; return exit code"]

There are explicit alternatives to the default work phase.
Within the default ``main()``, a selected command module's main callback takes precedence, followed by a callback
registered with ``setMainFn()``.
Either supplies the work phase instead of automatically starting parts and main-loop dispatch.
Overriding ``main()`` replaces that dispatch decision altogether.
Keep the default implementation when all you need is an event-driven main loop; the following sections show the startup
hooks that fit that design.

Starting Work in the Running Loop
=================================

The central startup pattern is to **prepare state, queue a startup callback through the intended target, then return**.
For the application's main loop, the target is ``app.loop()``.
This target exposes posting, scheduling, and loop services.
A derived application can access the full loop through the protected ``mainEventLoop()`` method when it needs to control
dispatch directly.
Calling :cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` on it queues the callback without executing it
inline.
When the lifecycle reaches main-loop dispatch, the loop can execute that callback in its own context.
This does not promise that your startup callback will be the first event dispatched.

Being on the main thread is not the same as being inside the running main loop.
``Application::initialize()`` and its command-line hooks run on that thread before dispatch begins.
Obtaining ``app.loop()`` gives you a target for queued work; it does not enter the loop or establish the current loop
context.
That distinction matters when you create event sources or use APIs that obtain their owner or services from the current
loop.
Do that setup inside the queued callback, where the intended loop is current, and retain the created resources for as
long as the operation needs them.

.. mermaid::

    sequenceDiagram
        participant A as Application lifecycle (main thread)
        participant Q as app.loop() queue
        participant D as Main-loop dispatch (same thread)
        A->>Q: invoke(startup callback)
        Note over A,Q: Callback is queued, not called here
        Q-->>A: invoke() returns
        Note over A: Hook returns and remaining startup succeeds
        A->>D: Default main enters runMainEventLoop()
        D->>Q: Dispatch accepted work
        Q-->>D: Execute startup callback
        Note over D: Current loop context is now available

Deferral also changes what your callback may safely capture.
Capture small configuration values by value, or store configuration in an application-owned object before queuing work.
A reference to a local variable in a startup hook becomes invalid when that hook returns.
Borrowing ``this`` or an object in the C++ entry point is appropriate when its lifetime encloses ``app.run()`` and every
callback that uses it.
For workers, that lifetime must also extend through shutdown and joining.
See :doc:`callback_lifetimes` for more elaborate ownership patterns.

Using Application Without a Derived Class
=========================================

For a small program, an ``el::Application`` instance and callbacks are enough.
If startup does not need parsed options, register a function with ``setInitializeFn()`` before calling ``run()``.
The default ``initialize()`` calls that function at the beginning of the lifecycle, before option registration and
parsing.
Prepare your state there and use ``app.loop()->invoke(...)`` to defer loop-dependent initialization.
Leave ``setMainFn()`` unset so the default main supplies dispatch.

When startup depends on command-line configuration, add an :cpp:class:`OptionSet <erbsland::options::OptionSet>` to
``app.options()`` and register its ``setPostParsingFn()`` callback.
This callback runs during command-line parsing, after successful parsing and option-value validation, before application
main.
Validate the relationships or constraints your application needs, retain the required configuration, and queue startup
only after that validation succeeds.
An :cpp:class:`OptionError <erbsland::options::OptionError>` reports invalid input through the framework's command-line
diagnostics.

Use the values passed to the post-parsing callback.
At this point ``app.optionValues()`` has not yet been populated with the parser's result.
Copy the values needed by the queued work instead of capturing the callback's parameter by reference.
If help, version output, or an error ends startup before main, the default main loop is never entered, so a queued
main-loop callback does not start business operations on those paths.

.. erbsland-in-practice-begin:: Template: Direct Initialization

This template supplies an application instance and an owned work object whose lifetime encloses ``run()``.
The framework calls the initialization function and later dispatches ``begin()`` on its main loop.
You supply the operations in ``begin()``, retain their sources in ``Work``, and connect their final completion to
``complete()``.

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/DirectInitialization/main.cpp
    :show: f:main
    :sha256: b37775b5e56a24298f0e9485e60d61189feaf560f7fb2c5c01476cebaa7b1710

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        auto work = demo::Work{app};
        app.setInitializeFn([&app, &work]() -> void {
            // Queue startup; the default main enters dispatch later.
            app.loop()->invoke([&work]() -> void { work.begin(); });
        });
        return app.run();
    }

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/DirectInitialization/Work.hpp
    :sha256: 1c773509d9943248ad0d09b998531aac130371a1ecf263b1afaa78a8f545267d

.. code-block:: cpp

    /// Own the operations started on the application's main loop.
    class Work final {
    public:
        /// Borrow the application for this work object's lifetime.
        /// @param app Application whose lifetime encloses this object.
        explicit Work(el::Application &app) : _app{app} {}

    public: // event work
        /// Start business operations in the running main loop.
        void begin() {
            // TODO: Create and retain your event sources here.
            // Connect their final successful completion to complete().
        }
        /// Finish after all required operations have completed on this loop.
        void complete() { _app.quit(); }

    private:
        el::Application &_app; ///< Application outlives this owner.
    };

.. erbsland-demo-end::

The completion hook calls ``quit()`` only when your operations finish.
As supplied, this compile-only template waits for work that you have yet to implement.
Templates are compiled and linked, but never executed for documentation or validation.

.. erbsland-in-practice-end::

.. erbsland-in-practice-begin:: Template: Direct Configuration Callback

This alternative declares a required text option, rejects blank input using the supplied parsed values, and captures the
validated string by value for dispatch.
The framework provides parsing, diagnostics, and the default main loop.
You supply the operations that consume the retained input and call ``complete()`` when they finish.

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/DirectOptions/main.cpp
    :show: f:main
    :sha256: 14c1983d18b96c810d09a9b986bbab051e55b201fbd3b428e47c93f1c2dfba1d

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        auto work = demo::Work{app};
        auto options = el::OptionSet::create();
        options->addOption({"--input"_el, "input"_el})
            .setType(el::OptionType::Text)
            .setRequired()
            .setHelpTitle("Input to process."_el);
        options->setPostParsingFn([&app, &work](const el::OptionValuesPtr &values) -> void {
            const auto input = values->getText("input"_el).trimmed();
            if (input.isEmpty()) {
                throw el::OptionError{"Invalid Input"_el, "Input must not be blank."_el, values->value("input"_el)};
            }
            app.loop()->invoke([&work, input]() -> void { work.begin(input); });
        });
        app.options()->addSet(options);
        return app.run();
    }

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/DirectOptions/Work.hpp
    :sha256: 30c2b0495fa178a389af5c63e97c20607e9a79b2816076dafbe89aef506f9b51

.. code-block:: cpp

    /// Own the operations started on the application's main loop.
    class Work final {
    public:
        /// Borrow the application for this work object's lifetime.
        /// @param app Application whose lifetime encloses this object.
        explicit Work(el::Application &app) : _app{app} {}

    public: // event work
        /// Start business operations in the running main loop.
        /// @param input Validated input retained for asynchronous operations.
        void begin(el::String input) {
            _input = std::move(input);
            // TODO: Create and retain your event sources here.
            // Connect their final successful completion to complete().
        }
        /// Finish after all required operations have completed on this loop.
        void complete() { _app.quit(); }

    private:
        el::Application &_app; ///< Application outlives this owner.
        el::String _input;     ///< Input retained for callbacks.
    };

.. erbsland-demo-end::

.. erbsland-in-practice-end::

Starting a Derived Application
==============================

Deriving from ``Application`` puts the same lifecycle in named overrides and gives your state a natural owner.
Override ``initialize()`` to prepare state that does not depend on parsed options, and queue a call to your startup
method through ``loop()->invoke(...)``.
Capturing ``this`` lets that callback use application members when the default main loop begins dispatching.
Keep resources in members so they survive the initialization hook's return.

For an application that needs configuration, override ``registerCommandLineOptions()`` to declare its options and
``handleCommandLineOptions()`` to consume successfully parsed values.
Validate those values, store the configuration in members, then queue the startup method.
The handling hook is skipped for help, version output, and parsing failures.
You do not need to override ``parseCommandLine()`` for ordinary validation; that override is for replacing parsing
behavior itself.
Choose the hook that has the information your startup requires, and queue that startup once.

In both designs, retain the default ``main()``.
If your application genuinely needs a procedural phase before or after main-loop dispatch, an overridden ``main()`` can
perform that phase, call the protected ``runMainEventLoop()``, then return its ``ExitCode`` after any synchronous
follow-up.
Queue loop-dependent setup before entering it, and provide a completion path that requests ``quit()``.
``runMainEventLoop()`` enters the application loop; it does not perform the default main's command selection or
automatic part startup.
Calling ``Application::main()`` from the override instead retains that complete dispatch behavior.

.. erbsland-in-practice-begin:: Template: Derived Initialization

The application owns the startup and completion methods, and the framework supplies the lifecycle and dispatch.
Add your resources as members, start operations in ``beginWork()``, and connect final completion to ``completeWork()``.
The cleanup hook is the place for final resource release after dispatch has ended.

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/DerivedInitialization/WorkApplication.hpp
    :sha256: 2ae4e599d2dfcac52ec26a84efa6f40004d7117071beb3363435ffbab8fca9ef

.. code-block:: cpp

    /// Own event-driven work while retaining Application's default main loop.
    class WorkApplication final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application
        void initialize() override {
            // Prepare application-owned state that does not require loop context.
            loop()->invoke([this]() -> void { beginWork(); });
        }
        void cleanup() noexcept override {
            // TODO: Release application resources; initialization may have failed partway.
            // Main-loop callbacks cannot run here.
        }

    private:
        /// Create event sources in the context of the running main loop.
        void beginWork() {
            // TODO: Start and retain your business operations here.
            // Connect their final successful completion to completeWork().
        }
        /// Request shutdown after all required work has finished on this loop.
        void completeWork() { quit(); }
    };

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/DerivedInitialization/main.cpp
    :show: f:main
    :sha256: f17e83b9934525224e90cf8408db4a7bebc9f46b952b3af1bcaa1721775d39a9

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = demo::WorkApplication{argc, argv};
        return app.run();
    }

.. erbsland-demo-end::

.. erbsland-in-practice-end::

.. erbsland-in-practice-begin:: Template: Derived Option Handling

Here the application declares and validates its input in separate hooks.
The framework passes parsed values to ``handleCommandLineOptions()`` before calling the inherited main.
You implement business operations using the stored ``_input`` and complete them through ``completeWork()``.

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/DerivedOptions/WorkApplication.hpp
    :sha256: 1ab2ded6a1aa964a3c13ecac8c8f854cfd5a154d2112c0c3b8a92f493a095e39

.. code-block:: cpp

    /// Own event-driven work while retaining Application's default main loop.
    class WorkApplication final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            options->addOption({"--input"_el, "input"_el})
                .setType(el::OptionType::Text)
                .setRequired()
                .setHelpTitle("Input to process."_el);
        }
        void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
            _input = values->getText("input"_el).trimmed();
            if (_input.isEmpty()) {
                throw el::OptionError{"Invalid Input"_el, "Input must not be blank."_el, values->value("input"_el)};
            }
            loop()->invoke([this]() -> void { beginWork(); });
        }
        void cleanup() noexcept override {
            // TODO: Release application resources; initialization may have failed partway.
            // Main-loop callbacks cannot run here.
        }

    private:
        /// Create event sources in the context of the running main loop.
        void beginWork() {
            // TODO: Start and retain your business operations here.
            // Connect their final successful completion to completeWork().
        }
        /// Request shutdown after all required work has finished on this loop.
        void completeWork() { quit(); }

    private:
        el::String _input; ///< Validated configuration retained for event callbacks.
    };

.. erbsland-demo-end::

.. erbsland-in-practice-end::

Running Worker Loops from a Procedural Main
===========================================

A procedural application can use event-driven work without dispatching its main loop.
Override ``main()`` for the synchronous workflow and create a managed worker with ``createEventThread()``.
Retain the returned :cpp:class:`ManagedEventThread <erbsland::event::ManagedEventThread>` pointer in the owner: creation
alone does not start the thread, and the application's management of workers does not replace your ownership.
Call ``start()``, then queue initialization through ``worker->loop()->invoke(...)``.
That callback runs in the worker loop's context, even while procedural main is waiting or doing other work.

Before joining the worker, establish how it will finish.
The worker's final completion callback can call ``worker->quit()`` after all required operations have completed.
The owner calls ``join()`` and keeps both the worker and its borrowed state alive until joining finishes.
Calling ``join()`` does not request shutdown by itself, and joining a loop that continues waiting for events will keep
the caller waiting too.
Read worker-owned results from procedural main only after synchronization such as joining.

Required progress cannot depend on a reply queued to an application main loop that is not dispatching.
Finish on the worker, or deliver completion to another loop that is actually running.
Set ``EventLoopErrorPolicy::FailApplication`` in the ``EventLoopOptions`` supplied to ``createEventThread()`` so a
callback failure stops the managed work and reaches the application's error boundary.
This gives a waiting owner a failure path as well as a successful completion path; returning success from procedural
main does not discard a managed worker failure retained by the framework.
For multiple workers and result-routing designs, continue with :doc:`processing_work_on_event_threads`.

.. erbsland-in-practice-begin:: Template: Procedural Main with a Worker

The application retains ``_worker`` through joining and cleanup.
The framework runs its loop and propagates managed failures.
You implement ``beginWork()`` on that worker and arrange for its final completion to call ``completeWork()``.
Until you provide that completion, the template's ``join()`` would wait indefinitely; it is compiled, never run.

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/ProceduralWorker/WorkApplication.hpp
    :sha256: cae33c860bc2fa0d651b936e7d6794090c5a191453d727f92f03d09a96f92777

.. code-block:: cpp

    /// Run event work on a retained worker while procedural main waits for completion.
    class WorkApplication final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application
        auto main() -> el::ExitCode override {
            auto options = el::EventLoopOptions{};
            options.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication);
            _worker = createEventThread(options);
            _worker->start();
            _worker->loop()->invoke([this]() -> void { beginWork(); });
            // completeWork() must quit the worker before this join can return.
            _worker->join();
            // Read worker-owned results only after joining. No main loop was needed.
            return el::ExitCode::success();
        }
        void cleanup() noexcept override {
            // The framework also stops and joins managed workers on failure.
            _worker.reset();
            // TODO: Release remaining resources, tolerating partial initialization.
        }

    private:
        /// Start operations inside the worker loop, never on the waiting main thread.
        void beginWork() {
            // TODO: Create and retain your worker's business operations here.
            // Connect their final successful completion to completeWork().
            // Do not route required completion through the application's idle main loop.
        }
        /// Finish on the worker; the owner on the main thread performs the join.
        void completeWork() { _worker->quit(); }

    private:
        el::ManagedEventThreadPtr _worker; ///< Owned by the application through shutdown and joining.
    };

.. erbsland-demo-end::

.. erbsland-in-practice-end::

.. _event-application-parts:

Starting Event Work in an Application Part
==========================================

An :cpp:class:`ApplicationPart <erbsland::core::ApplicationPart>` is a component whose lifecycle and dedicated event
loop are managed by the application framework.
Register the part type with ``app.registerPart<YourPart>()`` before ``app.run()``.
Registration supplies the manager with the type's factory and stable identifier; it does not start a thread immediately.
After successful application startup, the default main starts parts whose ``automaticStart()`` permits it, in dependency
order, and also runs the application's main loop.
The manager creates and retains each part and its dedicated loop.
You do not create a thread or call a loop-running method for the part yourself.

**A part's onInitialize() executes as a callback in its own event loop.** This differs from
``Application::initialize()``, which runs before application main-loop dispatch.
Create loop-bound resources and connect their handlers in ``onInitialize()``.
After initialization returns, the manager publishes ``Running`` and queues ``onRunning()`` on the part's loop.
Begin business operations directly in ``onRunning()``; the framework provides the deferred callback.

The manager can start dependent parts once initialization has completed, without waiting for ``onRunning()`` to finish.
If another component must wait for an asynchronous service to become usable, provide an explicit readiness notification
or service contract.
See :doc:`using_event_loops_in_application_parts` for readiness, dependencies, and coordinated part shutdown.

.. erbsland-in-practice-begin:: Template: Registered Application Part

Registration leaves automatic startup enabled.
The framework supplies the factory call, dedicated thread, loop context, and lifecycle hooks.
You create and retain resources in ``onInitialize()`` and start their operations in ``onRunning()``.
This template's ``completeWork()`` ends the whole application once the component's required work has finished.

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/RegisteredPart/main.cpp
    :show: f:main
    :sha256: 6cc5fbadd4177138fd18db855ccefe24fc75fb1102d7d1bd294b76ffd9b10cca

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.registerPart<demo::WorkPart>();
        return app.run();
    }

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/RegisteredPart/WorkPart.hpp
    :sha256: 6fb88afb85f93abcde2b8687768c0876b8369249b02a19ccacfaac5079bb06d3

.. code-block:: cpp

    /// Let the application framework own a component and its dedicated event loop.
    class WorkPart final : public el::ApplicationPart {
    public: // registration
        /// Identify this component in the application's part registry.
        [[nodiscard]] static auto partIdentifier() -> el::ApplicationPartIdentifierPtr {
            static const auto identifier = el::ApplicationPartIdentifier::create("application.work"_el);
            return identifier;
        }
        /// Create the component retained by its manager.
        [[nodiscard]] static auto create() -> std::shared_ptr<WorkPart> { return std::make_shared<WorkPart>(); }

    protected: // implement ApplicationPart
        void onInitialize() override {
            // TODO: Create and retain event sources on this part's loop and connect their handlers.
            // Connect the final successful completion of your work to completeWork().
        }
        void onRunning() override {
            // TODO: Start your business operations after this part enters Running.
            // Report asynchronous service readiness separately if other parts require it.
        }
        void cleanup() noexcept override {
            // TODO: Release part-owned resources after its loop exits.
            // Initialization may have failed partway.
        }

    private:
        /// Finish the whole application after this component's required work completes.
        void completeWork() { el::Application::instance().quit(); }
    };

.. erbsland-demo-end::

.. erbsland-in-practice-end::

Finishing the Application and Releasing Resources
=================================================

Successful completion begins in your application's own work: decide when all required operations have finished, then
request ``Application::quit()`` from a completion callback.
For a procedural worker, quitting that worker lets the owning main finish its join and return.
A quit request does not interrupt a callback already executing, and it does not make unfinished asynchronous operations
complete successfully.
Finish or cancel those operations while their owner loop can still process the callbacks they require.

The application coordinates shutdown of managed parts and threads before completing its lifecycle.
Parts receive their stopping hooks; a part that needs asynchronous shutdown completes that work on its loop and calls
``completeStopping()`` when it is ready to stop.
The default stopping hook completes immediately.
Managed worker threads are requested to quit and joined, and application ``cleanup()`` runs after the work phase and
managed shutdown have ended.

Application ``cleanup()`` runs outside main-loop dispatch.
It is for final resource release, not for queuing operations and waiting for their callbacks.
It is also called on failure, including when initialization was only partly completed, so it must tolerate resources
that were never created and must not throw.
Use owned members and ordinary C++ destruction where possible; override cleanup when resources need release before
``run()`` returns.
A directly instantiated application can similarly keep resources in an owner whose lifetime encloses ``run()``.

An :cpp:class:`ApplicationError <erbsland::core::ApplicationError>` thrown by an application startup hook reaches the
error boundary in ``run()`` directly.
An ``ApplicationError`` escaping a main-loop callback is retained by the application loop, stops dispatch, and is passed
back to that same boundary after managed shutdown.
The framework performs cleanup, displays the diagnostic, and returns the error's exit code.
Managed workers configured with ``FailApplication`` use this application failure path too.
You can therefore report a startup failure from the deferred callback without adding a separate catch-and-print layer.

For cancellation, pending callbacks, and shutdown ordering in larger workflows, see :doc:`shutting_down_event_work`.
For operation error callbacks and the other event-loop failure policies, see :doc:`handling_event_errors`.
