..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: System Services; Reference
    single: System Information
    single: Operating System
    single: CPU Architecture
    single: Process Information
    single: Process Identifiers
    single: Environment Variables
    single: User Lookup
    single: Subprocesses

***************
System Services
***************

System Information
==================

Introduction
------------

The free functions in ``erbsland::system::info`` provide small machine-wide queries without exposing native types.
With the default flattened namespace, the same functions are available through ``erbsland::sys_info`` and
``el::sys_info``.
This deliberate alias avoids the ambiguous ``erbsland::info`` name.

``operatingSystem()`` classifies the current host operating-system family, and ``cpuArchitecture()`` reports the native
host architecture rather than an emulated executable ABI such as Rosetta or WOW.

``logicalCpuCount()`` is an affinity-aware thread-count hint where the platform supports that query.
It falls back through the platform's active-CPU count and the standard-library hardware concurrency value and always
returns at least one.
The count is not a promise of dedicated cores or a representation of a container CPU quota.

``OperatingSystem`` and ``CpuArchitecture`` are comparable smart-enum values with stable lowercase ``toString()``
representations.
Unsupported values use ``Unknown``.

Process Information
===================

Introduction
------------

``ProcessId`` is an opaque, platform-independent identifier created by system APIs.
Its default value is invalid, and validity only means that the value can represent a native process identifier; it does
not prove that a process currently exists.
Valid identifiers format as decimal text, while an invalid identifier formats as an empty string.

``ProcessInfo`` eagerly loads one independently owned snapshot for the current process or a supplied ``ProcessId``.
Every accessor reads that snapshot without another platform query, and ordinary copies keep independent snapshot data.
Call ``reload()`` or ``reloadOrThrow()`` to replace all cached fields together.

Process Lifetime and Identifier Reuse
-------------------------------------

A process can exit while information is collected.
When the backend can prove that the process vanished, the result is a normal absent snapshot with ``exists()`` false.
The non-throwing reload also converts unexpected lookup failures into an absent snapshot, while ``reloadOrThrow()``
reports those failures as ``PlatformError`` after replacing the previous snapshot.

Reload follows the numeric identifier.
An operating system can reuse that identifier for a new process, so code that needs continuity must retain the old valid
``startTime()`` and compare it with the reloaded valid value.

.. code-block:: cpp

    auto info = erbsland::system::ProcessInfo{process.processId()};
    const auto originalStart = info.startTime();

    info.reload();
    const bool sameProcess = info.exists() && originalStart.isValid() &&
                             info.startTime().isValid() && info.startTime() == originalStart;

Partial Access
--------------

An existing process can expose only part of its information because of platform permissions or native limitations.
The non-throwing accessors return the normal invalid sentinel for an unavailable field: an empty ``Path``, invalid
``ProcessId`` or ``DateTime``, or empty ``UserId``.
Each throwing accessor reports the native diagnostic cached when the snapshot was loaded; it does not perform a second
lookup.

The first version intentionally excludes command lines, working directories, environment data, per-process CPU
architecture, and mutable resource-usage metrics.

Environment Variables
=====================

Introduction
------------

``EnvironmentVariables`` provides portable access to the environment of the current process.
Each instance uses the native backend for the current operating system by default.
A custom backend can be supplied for isolated tests or specialized hosts.

Environment entries are process-wide.
Changes are visible to subsequent native lookups and to child processes that inherit the environment.
Names retain the native platform's case-sensitivity rules.

Reading Variables
-----------------

Use ``get(name)`` when absence must be distinguished from an empty stored value.
It returns ``std::nullopt`` for a missing variable or a failed lookup.
The two-argument overload returns its explicit fallback in these cases.

``getOrThrow(name)`` returns an empty string for a stored empty value and throws ``PlatformError`` if the variable does
not exist or the native lookup fails.

Writing and Removing Variables
------------------------------

``set()`` and ``remove()`` report failure through their boolean result.
Their ``OrThrow`` variants preserve platform failure details in ``PlatformError``.
Removal is idempotent, so removing a variable that is already absent succeeds.
Setting an empty value stores an empty value; it does not remove the entry.

Names must not be empty or contain an equals sign or embedded null byte.
Values must not contain embedded null bytes.
The throwing methods report these portable input errors as ``ParameterError``.

User Lookup
===========

Introduction
------------

The ``system`` namespace provides application-shared services that expose small, portable wrappers around native
operating-system facilities.

Native failures use immutable ``PlatformErrorContext`` values.
The internal platform backends capture native details immediately and expose a portable ``PlatformErrorCategory`` for
domain diagnostics and actionable help.
``PlatformError`` is the concrete exception used when the native failure itself is the reported error.

``UserLookup`` resolves owner and group identifiers into display names and resolves names back to platform identifiers.
It caches successful lookups and is available through
:cpp:func:`Application::userLookup() <erbsland::core::Application::userLookup>` for code that needs a shared service
instance.

Subprocesses
============

Introduction
------------

``Subprocess`` starts and owns one operating-system child process without invoking a command shell.
The executable and each argument are passed as separate values, so shell quoting, expansion, redirection, and pipelines
are never applied.

An owned child is reaped when it exits.
Destroying a ``Subprocess`` that is still running first requests termination, waits briefly, then forcefully terminates
and reaps the child.
This makes the type suitable for helpers whose lifetime must not outlive an application scope or test.
``startDetached()`` is the explicit launch-and-forget operation; an internal waiter still prevents a zombie process on
POSIX systems.

Starting a Process
------------------

``start()`` requires a direct executable path and an optional ``StringList`` of arguments.
It does not search the ``PATH`` environment variable.
``SubprocessOptions`` can select a working directory, environment inheritance and changes, standard-input inheritance,
and output handling.

Environment changes are applied after optional inheritance.
A value assigns the variable and ``std::nullopt`` removes it.
Variable-name case sensitivity follows the operating system.

Waiting and Termination
-----------------------

``wait()`` blocks until the child exits.
The timed overload returns ``std::nullopt`` when its non-negative deadline expires.
``isRunning()`` performs a non-blocking status update.
Once observed, the ``SubprocessExitStatus`` remains available through ``exitStatus()``.
The ``processId()`` accessor returns the identifier assigned at launch and keeps returning it after the child exits.
A moved-from ``Subprocess`` returns an invalid identifier.
Use the process-information interfaces described above to inspect a running child through a portable snapshot.

On POSIX, ``terminate()`` sends ``SIGTERM`` and ``kill()`` sends ``SIGKILL``.
Windows has no equivalent graceful process signal, so both operations use native forced process termination.
Normal Windows termination is reported as an exit code; POSIX signal termination is reported separately.

Standard Streams
----------------

Standard output and standard error can independently be inherited, discarded, or captured.
Standard error can instead be merged into the selected standard-output destination.
Standard input can only be inherited or connected to an immediately closed null stream; interactive input piping is
intentionally outside this API.

Capture retains a bounded prefix while background readers continue draining the native pipes.
This prevents a verbose child from blocking after the capture limit is reached.
The default limit is one MiB per stream and the maximum configurable limit is 64 MiB.
Use the truncation accessors to detect discarded suffixes.
Detached processes cannot use capture because no owner remains to consume the result.

Interface
=========

.. doxygenclass:: erbsland::system::CpuArchitecture
    :members:
.. doxygenclass:: erbsland::system::EnvironmentVariables
    :members:
.. doxygenclass:: erbsland::system::FileIdentity
    :members:
.. doxygenclass:: erbsland::system::GroupId
    :members:
.. doxygenclass:: erbsland::system::GroupName
    :members:
.. doxygenclass:: erbsland::system::OperatingSystem
    :members:
.. doxygenclass:: erbsland::system::PlatformError
    :members:
.. doxygenclass:: erbsland::system::PlatformErrorCategory
    :members:
.. doxygenclass:: erbsland::system::PlatformErrorContext
    :members:
.. doxygenclass:: erbsland::system::ProcessId
    :members:
.. doxygenclass:: erbsland::system::ProcessInfo
    :members:
.. doxygenclass:: erbsland::system::Subprocess
    :members:
.. doxygenclass:: erbsland::system::SubprocessExitStatus
    :members:
.. doxygenclass:: erbsland::system::SubprocessOptions
    :members:
.. doxygenenum:: erbsland::system::SubprocessOutputMode
.. doxygenfunction:: erbsland::system::info::operatingSystem() noexcept -> OperatingSystem

.. doxygenfunction:: erbsland::system::info::cpuArchitecture() noexcept -> CpuArchitecture

.. doxygenfunction:: erbsland::system::info::logicalCpuCount() noexcept -> std::uint32_t
.. doxygenclass:: erbsland::system::UserId
    :members:
.. doxygenclass:: erbsland::system::UserLookup
    :members:
.. doxygenclass:: erbsland::system::UserName
    :members:
