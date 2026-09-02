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

Manager Access and Lifecycle
----------------------------

:cpp:class:`ApplicationPartManagerAccess <erbsland::core::ApplicationPartManagerAccess>` provides thread-safe state,
wait, and prepared-part lookup operations.
The mutable manager interface adds registration, preparation, command-line forwarding, asynchronous lifecycle requests,
callbacks, and ordered error retrieval.

Manager and part state callbacks execute on the manager's control event source.
Every started part receives a separate event thread.
Dependencies gate startup and reverse the shutdown order.

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
The heading uses the application's ``log.LastErrorDumpTitle`` display-text entry.
See :doc:`/topics/log/using_logging` for routing, trace sections, and standalone managers.

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
