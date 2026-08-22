*******************************
Core Application API Guidelines
*******************************

Core Semantics
==============

Application Lifecycle
---------------------

.. code-block:: text

    application = process-lifetime owner of shared Core services
    initialization = application setup before option registration and parsing
    main phase = application work or main event-loop execution
    cleanup = non-throwing finalization after success or handled library failure
    quit = coordinated exit-code request for the main loop and managed event threads
    application part = one-shot dependency-aware component with a dedicated event thread
    part identifier = stable case-sensitive name resolved within one manager
    part dependency = lifecycle availability relation that reverses shutdown order
    control event source = serialized owner of part graph and lifecycle decisions

Process Boundaries
------------------

.. code-block:: text

    native arguments = borrowed process values retained for the application lifetime
    converted arguments = owning UTF-8 values used within the application
    application instance = single active process-wide application object
    library failure = rendered diagnostic and controlled nonzero exit
    foreign failure = outside the handled Core exception boundary

Primary Types
=============

.. code-block:: text

    Application // process application lifecycle and shared Core service owner
    ApplicationPartManager // detached application-part graph and lifecycle owner

Secondary Types
===============

.. code-block:: text

    ApplicationInfo // application name, version, author, copyright, and license metadata
    CommandLineArguments // owning UTF-8 process argument list
    InitializeFn, MainFn // functional lifecycle customization callbacks
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
    o.enableTerminal() // create and retain advanced terminal integration
    o.releaseOptions() // release startup-only option definitions

Customization Patterns
======================

.. code-block:: text

    o.initialize()/registerCommandLineOptions(options) // customize startup before parsing
    o.parseCommandLine() // customize command-line parsing
    o.main() -> unit::ExitCode // customize application work
    o.cleanup() // customize non-throwing finalization
    o.setInitializeFn/setMainFn(function) // customize lifecycle without deriving
    o.createAndInitializeTerminal() -> cterm::TerminalPtr // customize terminal construction

Application-Part Patterns
=========================

.. code-block:: text

    T::partIdentifier() -> ApplicationPartIdentifierPtr // publish a stable part or interface identity
    T::dependencies() -> ApplicationPartIdentifierList // declare static lifecycle dependencies
    T::create() -> shared_ptr<T> // construct a prepared concrete part
    o.prepare() // close registration, validate the graph, and construct parts
    o.start([identifier])/stop([identifier]) // request asynchronous one-shot lifecycle transitions
    o.waitForRunning([identifier])/waitForStopped([identifier]) -> bool // wait outside managed event threads
    o.registerCommandLineOptions(options)/parseCommandLine(values) // synchronously forward option phases
    o.setErrorHandler(handler) // select Continue or StopAll after part failures
    o.hasError()/takeError() -> T // inspect and consume the ordered error queue

Application Service Patterns
============================

.. code-block:: text

    o.info()/commandLineArguments()/optionValues() -> T // access startup metadata and parsed input
    o.random()/secureRandom() -> random::Random& // access shared random generators
    o.displayText()/setDisplayTextMap(map) // inspect or replace application display text
    o.userLookup() -> system::UserLookup& // access shared identity lookup
    o.resources() -> const resource::Resources& // access compiled resources through a lazy shared manager
    o.terminal()/systemOutputStyle() -> T // access terminal integration and presentation
    o.events()/eventRegistry()/createEventThread() -> T // access shared event services

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
