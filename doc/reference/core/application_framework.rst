..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application Framework; Reference
    single: Application
    single: Application Part
    single: ApplicationPartManager

*********************
Application Framework
*********************

Application Lifecycle
=====================

See :doc:`/topics/core/choosing_an_application_design` for the function-based, event-driven, procedural, command-style,
and application-part designs supported by this lifecycle.

``run()`` initializes the application, registers options, and parses the command line.
After successful parsing it calls ``handleCommandLineOptions(values)``, forwards the values to prepared application
parts, and calls ``main()``.
The hook receives non-null parsed values and does nothing by default.
Help, version output, module overview, and parsing errors skip both the hook and ``main()``.

Override ``handleCommandLineOptions()`` to validate paths, apply configuration, or queue work that depends on parsed
values while retaining the default main dispatch.
That dispatch selects an option-module main function, then a function installed through ``setMainFn()``, and otherwise
starts registered parts and enters the main event loop.
Override ``parseCommandLine()`` only to replace the parsing behavior itself.
Exceptions from the hook use the same reporting and cleanup boundary as other lifecycle methods.

:cpp:class:`Application <erbsland::core::Application>` retains non-owning access to the original narrow or wide
``argv`` vector supplied to its constructor.
After option parsing, every suffix reported as sensitive text is overwritten in place with one star per existing byte or
code unit.
The terminating null and native buffer size are preserved.
Its converted :cpp:type:`CommandLineArguments <erbsland::core::CommandLineArguments>` list uses exactly five stars for
each sensitive value.
Masking occurs for successful parsing and parser or validator errors.

The native pointers must remain valid for the application lifetime.
This cleanup only reduces secrets retained in process memory; it cannot retract command-line values already exposed
through process listings, the shell, operating-system facilities, logs, or earlier application code.

Final application destruction flushes both standard output streams after log and terminal cleanup, even when no terminal
was enabled.
This drains writes queued on I/O workers before process teardown can discard them.
Each stream is flushed independently using its configured timeout; a flush failure does not prevent cleanup of the other
stream or escape the application destructor.

Exception Handling
==================

``Application::run()`` handles ``RuntimeError`` from lifecycle methods and the managed main loop by shutting down
managed work, calling ``cleanup()``, rendering the diagnostic to standard error, and returning a failure exit code.
``ApplicationError`` supplies its own exit code; other runtime errors use ``ExitCode::failure()``.
``LogicFailure`` is logged when logging is configured, then rethrown after cleanup.
Other exceptions also propagate after cleanup.
These fatal failures are not converted into ordinary application diagnostics and successful returns.

``runMainEventLoop()`` installs the default handler on every entry, replacing any previously installed main-loop
handler.
It stops dispatch on the first runtime failure and delivers that failure to the application boundary after managed
worker shutdown.
A derived ``main()`` can instead install a custom handler and drive ``mainEventLoop().run()`` directly; that override
owns consumption of stored errors, reporting, and the exit code it returns.
The surrounding application lifecycle still handles managed-thread shutdown and cleanup.

Managed event threads use ``Store`` unless configured otherwise.
``FailApplication`` forwards worker runtime failures to the application, retaining the first failure and requesting
shutdown of all managed event threads.
See :doc:`/topics/event/handling_event_errors` for local recovery, final notifications, policies, and joining.

Terminal Output
===============

``enableTerminal(flags)`` creates and initializes an application-owned terminal with an initial size of 80×25.
``enableTerminal(size, flags)`` supplies an explicit size.
Native backends detect the available screen; ``TerminalFlag::VirtualTty`` preserves the supplied dimensions and emits
ANSI through stdout even when redirected.
Repeated calls leave the enabled terminal and its settings unchanged.

Interactive and virtual terminals route Core's standard output through the terminal with default colors and standard
error in bright red.
The application restores the screen and standard-stream routing during cleanup.
Customize construction through ``createAndInitializeTerminal(size, flags)`` while preserving the requested settings.
See :doc:`/reference/cterm/terminal` for virtual input and screen dimensions.

Service Ownership
=================

The public ``application()`` function provides access to the current application and creates a temporary application
when called before explicit construction.
Construct your application before configuring its services so their setup follows a clear lifecycle.

Accepted background operations retain the services they need until their native work finishes, including after the
application object is destroyed.
This keeps protected-memory access and secure randomness associated with the originating application.
It does not extend the lifetime of references to the public application object.
Resource loaders and HTTP resource handlers retain their resource provider independently; see
:doc:`/reference/text/documents_and_rendering` and :doc:`/reference/network/http_server`.

Service Lifecycle
=================

``enableServiceLifecycle()`` lets the same executable run interactively, as a foreground POSIX daemon, or as a Windows
own-process service.
On Linux and macOS, SIGTERM and SIGINT request the regular graceful application shutdown.
On Windows, the application connects to the Service Control Manager when launched as a service and otherwise continues
as an interactive console application.
SCM stop and shutdown controls and interactive Ctrl+C or Ctrl+Break requests use the same application-part and
event-loop shutdown path.

Startup is reported as complete immediately before main work begins.
A slow initializer can call ``reportStartupPending(expectedTime)`` before that point and publish genuine progress with
additional calls.
It must call ``reportStartupComplete()`` when the application is ready.
The default application-part lifecycle automatically keeps startup pending until all automatically started parts reach
the running state.

See :doc:`/topics/core/service_lifecycle` for complete examples and lifecycle details.

Application Parts
=================

Application parts are dependency-aware, one-shot components with dedicated event threads.
The detached
:cpp:class:`ApplicationPartManager <erbsland::core::ApplicationPartManager>` owns their graph and runtime, while
:cpp:class:`Application <erbsland::core::Application>` can provide the same manager as a lazy application service.
See :doc:`/topics/core/applications_from_parts` for application integration,
:doc:`/topics/core/application_parts` for part design and lifecycle guidance, and
:doc:`/topics/core/detached_application_parts` for standalone manager operation.

Identifiers and Registration
----------------------------

:cpp:class:`ApplicationPartIdentifier <erbsland::core::ApplicationPartIdentifier>` stores a stable, case-sensitive
name.
A manager resolves identifiers by name, so separately created identifiers with identical names address the same
registered part.
The manager rejects invalid dependency graphs before constructing any part.

``ApplicationPartIdentifierPtr`` and ``ApplicationPartIdentifierList`` provide the shared identifier and dependency list
aliases.
``ApplicationPartPtr``, ``ApplicationPartManagerPtr``, ``ApplicationPartManagerAccessPtr``, and
``ApplicationPartManagerAccessWeakPtr`` provide the corresponding ownership aliases.

Part Interfaces
---------------

:cpp:class:`ApplicationPart <erbsland::core::ApplicationPart>` provides lifecycle hooks, command-line forwarding, the
dedicated event target, and read-only manager access.
Public service interfaces publish the same static identifier and can be combined with the implementation through
:cpp:class:`ApplicationPartWithInterface <erbsland::core::ApplicationPartWithInterface>`.

Part Lifecycle Hooks
--------------------

These protected hooks customize the part's event work and finalization.
See :doc:`/topics/event/using_event_loops_in_application_parts` for their execution contexts and source ownership.

.. doxygenfunction:: erbsland::core::ApplicationPart::automaticStart
.. doxygenfunction:: erbsland::core::ApplicationPart::shutdownTimeout
.. doxygenfunction:: erbsland::core::ApplicationPart::onInitialize
.. doxygenfunction:: erbsland::core::ApplicationPart::onRunning
.. doxygenfunction:: erbsland::core::ApplicationPart::onStopping
.. doxygenfunction:: erbsland::core::ApplicationPart::cleanup
.. doxygenfunction:: erbsland::core::ApplicationPart::completeStopping

Manager Access and Lifecycle
----------------------------

:cpp:class:`ApplicationPartManagerAccess <erbsland::core::ApplicationPartManagerAccess>` provides thread-safe state,
wait, and prepared-part lookup operations.
The mutable manager interface adds registration, preparation, command-line forwarding, asynchronous lifecycle requests,
callbacks, and ordered error retrieval.

Manager and part state callbacks execute on the manager's control event loop.
Every started part receives a separate event thread.
``onInitialize()`` runs as an event callback while the part is ``Starting``.
Successful initialization causes the manager to publish ``Running`` and queue ``onRunning()`` as a separate callback on
the part's loop.
The manager can start dependent parts and satisfy ``waitForRunning()`` before ``onRunning()`` finishes.
Callbacks queued during initialization can run before ``onRunning()``.
``Running`` establishes completed initialization, not completion of asynchronous service setup.
Dependencies gate startup and reverse the shutdown order.
``onStopping()`` runs as an event callback and begins graceful shutdown; ``completeStopping()`` requests loop
termination when that work completes.
``cleanup() noexcept`` executes on the part thread after the loop exits, including after handled callback failures.
Command-line registration and ``handleCommandLineOptions(values)`` run synchronously before part startup.

Lifecycle Observers
-------------------

``ApplicationPartManager`` derives from :cpp:class:`EventSource <erbsland::event::EventSource>`.
Its ``ownerLoop()`` is the control target selected at creation: the application's main loop for ``app.partManager()``,
the supplied target for ``ApplicationPartManager::create(target)``, or a dedicated control thread's loop for
``create()``.
The target is bound before the factory returns and remains unchanged for the manager's lifetime.

``events()`` returns a stable, manager-owned
:cpp:class:`ApplicationPartManagerEventEditor <erbsland::core::ApplicationPartManagerEventEditor>`, derived from
:cpp:class:`EventEditor <erbsland::event::EventEditor>`.
Its ``source()`` retains the manager and ``target()`` retains its control target.
The editor reference itself is borrowed and remains valid only while the manager is alive.

Editor access, ``source()``, ``target()``, and observer registration are thread-safe and may be used before loop
dispatch begins.
Notifications always execute on the control loop, regardless of the registering thread.
This guarantee applies to the manager's editor; other event sources require owner-loop editing unless documented
otherwise.

State observers are independent subscriptions.
Retain the ``EventSubscription`` returned by ``events().addStateChanged()`` or ``events().addPartStateChanged()`` for as
long as the observer shall remain active.
Handled operational failures from observers are queued on the manager and trigger coordinated shutdown.
See :doc:`/topics/event/using_event_apis` for connecting callbacks and retaining subscriptions.

Compiled Resources
==================

:cpp:func:`Application::resources() <erbsland::core::Application::resources>` lazily creates the read-only compiled
resource manager.
The manager indexes statically linked descriptors on first access and caches decoded data and text independently.
An application with no compiled descriptors receives an empty manager.
See :doc:`/topics/resource/compiled_resources` for CMake integration and lookup examples.

Logging
=======

``log()`` lazily creates the application-wide log manager, and ``logStream()`` returns its root stream.
The application default routes information, warning, and error entries to its terminal-backed console writer.
``enableLastErrorDump()`` installs a retained-error writer that remains present across later configuration replacements.
By default, final cleanup displays its nonempty snapshot only after a nonzero application exit code.
Select :cpp:enumerator:`LastErrorDumpMode::Always <erbsland::core::LastErrorDumpMode::Always>` to display it after a
successful run as well.
The heading uses the application's ``log.last_error_dump_title`` display-text entry.
See :doc:`/topics/log/using_logging` for routing, trace sections, and standalone managers.

Application-part shutdown timeout expiry is reported to the manager as ``ApplicationError`` and follows the configured
part-error policy.
The original error remains available through the manager's error queue.

Interface
=========

.. doxygenclass:: erbsland::core::Application
    :members:

.. doxygenfunction:: erbsland::core::application() -> Application &
.. doxygenclass:: erbsland::core::ApplicationError
    :members:
.. doxygenclass:: erbsland::core::ApplicationInfo
    :members:
.. doxygenclass:: erbsland::core::ApplicationPart
    :members:
.. doxygentypedef:: erbsland::core::ApplicationPartErrorHandler

.. doxygentypedef:: erbsland::core::ApplicationPartManagerStateChangedFn

.. doxygentypedef:: erbsland::core::ApplicationPartStateChangedFn
.. doxygenclass:: erbsland::core::ApplicationPartCommandLine
    :members:
.. doxygenenum:: erbsland::core::ApplicationPartErrorAction
.. doxygenclass:: erbsland::core::ApplicationPartIdentifier
    :members:
.. doxygenclass:: erbsland::core::ApplicationPartManager
    :members:
.. doxygenclass:: erbsland::core::ApplicationPartManagerAccess
    :members:
.. doxygenclass:: erbsland::core::ApplicationPartManagerEventEditor
    :members:
.. doxygenenum:: erbsland::core::ApplicationPartManagerState
.. doxygenenum:: erbsland::core::ApplicationPartState
.. doxygenconcept:: erbsland::core::ApplicationPartInterface

.. doxygenconcept:: erbsland::core::ApplicationPartClass
.. doxygenclass:: erbsland::core::ApplicationPartWithInterface
    :members:
.. doxygentypedef:: erbsland::core::CommandLineArguments
.. doxygentypedef:: erbsland::core::InitializeFn
.. doxygenenum:: erbsland::core::LastErrorDumpMode
.. doxygentypedef:: erbsland::core::MainFn
