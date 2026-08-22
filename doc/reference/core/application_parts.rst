.. index::
    single: Application Part
    single: ApplicationPartManager

*****************
Application Parts
*****************

Application parts are dependency-aware, one-shot components with dedicated event threads.
The detached
:cpp:class:`ApplicationPartManager <erbsland::core::ApplicationPartManager>` owns their graph and runtime, while
:cpp:class:`Application <erbsland::core::Application>` can provide the same manager as a lazy application service.
See :doc:`/topics/core/application_parts` for a complete example and lifecycle guidance.

Identifiers and Registration
============================

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
===============

:cpp:class:`ApplicationPart <erbsland::core::ApplicationPart>` provides lifecycle hooks, command-line forwarding, the
dedicated event target, and read-only manager access.
Public service interfaces publish the same static identifier and can be combined with the implementation through
:cpp:class:`ApplicationPartWithInterface <erbsland::core::ApplicationPartWithInterface>`.

Manager Access and Lifecycle
============================

:cpp:class:`ApplicationPartManagerAccess <erbsland::core::ApplicationPartManagerAccess>` provides thread-safe state,
wait, and prepared-part lookup operations.
The mutable manager interface adds registration, preparation, command-line forwarding, asynchronous lifecycle requests,
callbacks, and ordered error retrieval.

Manager and part state callbacks execute on the manager's control event source.
Every started part receives a separate event thread.
Dependencies gate startup and reverse the shutdown order.

Interface
=========

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
