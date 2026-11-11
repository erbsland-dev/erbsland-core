*******************************
Core Application API Guidelines
*******************************

Core Semantics
==============

.. code-block:: text

    application = process-lifetime owner of shared Core services
    initialization = application setup before option registration and parsing
    main phase = application work or main event-loop execution
    cleanup = non-throwing finalization after success or handled library failure
    application part = one-shot dependency-aware component with a dedicated event thread
    part dependency = lifecycle availability relation between application parts

Primary Types
=============

.. code-block:: text

    Application // process application lifecycle and shared Core service owner
    ApplicationPartManager // detached application-part graph owner and lifecycle EventSource

Secondary Types
===============

.. code-block:: text

    ApplicationInfo // application name, version, author, copyright, and license metadata
    CommandLineArguments // owning UTF-8 process argument list
    InitializeFn, MainFn // functional lifecycle customization callbacks
    LastErrorDumpMode // condition for displaying retained errors during final cleanup
    ApplicationError // controlled application termination failure
    ApplicationErrorContext // structured application failure and exit-code context
    ApplicationPart // base class for one managed part implementation
    ApplicationPartIdentifier // immutable public part name with private manager-local cache
    ApplicationPartManagerAccess // thread-safe state, wait, and prepared-part lookup access
    ApplicationPartState, ApplicationPartManagerState // one-shot lifecycle states
    ApplicationPartErrorAction // failure policy decision

Lifecycle Patterns
==================

.. code-block:: text

    T([argc, argv]) // create the process application and retain the native argument boundary
    o.run() -> int // execute initialization, option parsing, main work, and cleanup
    o.quit([exitCode]) // request coordinated event-loop termination
    o.partManager() -> ApplicationPartManagerPtr // access the lazy application-integrated manager
    o.registerPart<T>() // register an application-part class before run
    o.part<T>() -> shared_ptr<T> // access a prepared part through its interface
    o.enableTerminal([size, flags]) // create and retain advanced terminal integration
    o.enableServiceLifecycle() // enable foreground-daemon and Windows service lifecycle integration
    o.reportStartupPending(expectedTime)/reportStartupComplete() // report delayed startup progress and readiness
    o.releaseOptions() // release startup-only option definitions

Customization Patterns
======================

.. code-block:: text

    o.initialize()/registerCommandLineOptions(options) // customize startup before parsing
    o.parseCommandLine() // replace command-line parsing
    o.handleCommandLineOptions(values) // handle successful parsing before part options and main dispatch
    o.main() -> unit::ExitCode // customize application work
    o.cleanup() // customize non-throwing finalization
    o.setInitializeFn/setMainFn(function) // customize lifecycle without deriving
    o.createAndInitializeTerminal() -> cterm::TerminalPtr // customize terminal construction
    o.mainEventLoop() -> event::EventLoop& // access the main loop from a derived application
    o.runMainEventLoop() -> unit::ExitCode // run the main loop from a derived application's main

Application-Part Patterns
=========================

.. code-block:: text

    T::partIdentifier() -> ApplicationPartIdentifierPtr // publish a stable part or interface identity
    T::dependencies() -> ApplicationPartIdentifierList // declare static lifecycle dependencies
    T::create() -> shared_ptr<T> // construct a prepared concrete part
    o.prepare() // close registration, validate the graph, and construct parts
    o.start([identifier])/stop([identifier]) // request asynchronous one-shot lifecycle transitions
    o.waitForRunning([identifier])/waitForStopped([identifier]) -> bool // wait outside managed event threads
    o.registerCommandLineOptions(options)/handleCommandLineOptions(values) // synchronously forward option phases
    o.setErrorHandler(handler) // select Continue or StopAll after part failures
    o.ownerLoop() -> event::EventTargetPtr // access the manager's fixed control and callback target
    o.events() -> ApplicationPartManagerEventEditor& // access the stable manager-owned editor from any thread
    o.loop() -> event::EventTargetPtr // access a started application part's event target
    o.onInitialize() // initialize a part in its event loop while Starting
    o.onRunning() // begin work in a separate queued callback after initialization and publication of Running
    o.onStopping()/completeStopping() // begin and complete graceful stopping in the part's event loop
    o.cleanup() // finalize on the part thread after its loop exits
    o.addStateChanged/addPartStateChanged(callback) -> EventSubscription // thread-safe registration through the editor
    o.hasError()/takeError() -> T // inspect and consume the ordered error queue

Application Service Patterns
============================

.. code-block:: text

    o.info()/commandLineArguments()/optionValues() -> T // access startup metadata and parsed input
    o.random()/secureRandom() -> random::Random& // access shared random generators
    o.displayText() -> i18n::DisplayTextPtr // access the application display-text registry
    o.userLookup() -> system::UserLookup& // access shared identity lookup
    o.resources() -> const resource::Resources& // access compiled resources through a lazy shared manager
    o.enableLastErrorDump([mode]) // retain recent errors and display them on failure or after every run
    o.terminal()/systemOutputStyle() -> T // access terminal integration and presentation
    o.loop()/eventRegistry()/createEventThread() -> T // access shared event services

Application Instance Patterns
=============================

.. code-block:: text

    application() -> Application& // access the active application instance
    T::instance() -> Application& // access the active application instance through the type
    T::linkWith(application) // link an independently static-linked module to the process instance
    T::libraryVersion()/libraryVersionText() -> T // inspect build-time Core version information

Metadata and Error Patterns
===========================

.. code-block:: text

    o.❮property❯()/set❮Property❯(value) // inspect or update application metadata
    T(reason-or-context[, cause, exitCode]) // create a controlled application failure
    o.context()/exitCode() -> T // inspect application failure details
    o.set❮Property❯(value) -> ApplicationErrorContext& // fluently build application failure context
