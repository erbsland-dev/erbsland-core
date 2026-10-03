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
``ProcessId`` or ``Timestamp``, or empty ``UserId``.
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

``getOrThrow(name)`` returns an empty string for a stored empty value, throws ``NotFoundError`` for an absent variable,
and preserves native lookup failures as ``PlatformError``.

Writing and Removing Variables
------------------------------

``set()`` and ``remove()`` report failure through their boolean result.
Their ``OrThrow`` variants preserve platform failure details in ``PlatformError``.
Removal is idempotent, so removing a variable that is already absent succeeds.
Setting an empty value stores an empty value; it does not remove the entry.

Names must not be empty or contain an equals sign or embedded null byte.
Values must not contain embedded null bytes.
The throwing variable methods report these portable syntax errors as ``ParseError``.

Reading and Editing PATH
------------------------

``paths()`` returns an ``el::PathList`` of valid, lexically normalized entries in search order.
Reading never changes the environment.
Duplicate paths and malformed entries are skipped.
The non-throwing form returns an empty list when PATH is missing or lookup fails; ``pathsOrThrow()`` preserves lookup
errors.

``prependPath(path)`` moves a path to the front, while ``appendPath(path)`` adds it only if absent.
``insertPath(index, path)`` accepts an ``ItemIndex`` in the deduplicated list, including its end.
The index refers to the list before removing an existing equivalent entry; moving an earlier entry adjusts the index.
``removePath(path)`` removes equivalent entries and ``removePathsIn(base)`` also removes descendants.
Every edit drops malformed entries and deduplicates the list, retaining the original spelling of other valid entries.
The boolean forms report failure; the ``OrThrow`` forms distinguish invalid arguments (``ParameterFailure``), backend
decoding errors (``ParseError``), and native failures (``PlatformError``).
A failed read does not cause an edit to replace PATH with a fallback value.

.. code-block:: cpp

    auto environment = el::EnvironmentVariables{};
    const auto directories = environment.pathsOrThrow();
    environment.prependPathOrThrow(el::Path::userHomeDirectory() / "tools/bin"_el);
    environment.removePathsInOrThrow(el::Path::userHomeDirectory() / "obsolete-sdk"_el);

Both environment APIs share separator, lexical comparison, deduplication, and malformed-entry rules described below.
These are process-wide read/modify/write operations.
Coordinate concurrent environment changes in application code; the methods do not provide an atomic edit across
independent environment writers.
An absent PATH remains absent after removals.
As with ``EnvironmentChanges``, removing all entries from an existing PATH stores an empty value, which represents the
current directory on subsequent reads.

Executable Candidates
---------------------

``executableCandidates(name)`` returns possible executable paths without accessing the filesystem.
A filename is combined with each valid PATH entry, in order; an absolute native path produces one candidate.
Relative paths containing directory components are rejected.
The non-throwing form returns an empty list on failure; ``executableCandidatesOrThrow()`` reports invalid input and
lookup failures.
On Windows, names without a suffix gain ``.exe``; an explicit suffix is retained.
This API supports native process launching, so it does not consult shell aliases, ``PATHEXT``, or implicit application
search directories.
Check each candidate's existence, file type, and executable permissions before selecting it.
Use ``Path::toNativeString()`` when passing path-valued arguments to a subprocess.

Environment Changes
===================

``EnvironmentChanges`` is a copyable description of changes to an unspecified base environment.
Default construction inherits every base variable unchanged.
``setInherit(EnvironmentInherit::None)`` starts with no inherited entries; ``All`` selects all entries.
``setInherit(StringList)`` replaces inheritance with a named selection, ignoring missing names.
An empty selection inherits nothing.

``setVariable(name, value)`` assigns after inheritance, and ``removeVariable(name)`` or its list overload removes
variables.
The latest action for each name wins under native name-comparison rules.
``clearVariableChange(name)`` cancels either action and restores inheritance behavior for that name.
``clearVariableChanges()`` preserves inheritance and PATH rules.
Explicit names and values must be valid UTF-8 with the same syntax constraints as subprocess environment entries;
invalid arguments immediately throw ``ParameterFailure``.
Empty values remain distinct from absent variables.

``apply(base)`` returns a new ``StringMap<String>`` without modifying the supplied map or accessing the process
environment.
``SubprocessOptions::environmentChanges()`` provides mutable and const access to the descriptor, and
``setEnvironmentChanges(changes)`` replaces it.
Subprocess preparation copies the descriptor; regular and detached launches read the parent environment when the child
is started.

Portable PATH Edits
-------------------

PATH processing follows inheritance and variable assignments/removals.
``removePath(path)`` removes every equivalent entry, while ``removePathsIn(base)`` removes the base itself and its
descendants using complete path components.
Comparison normalizes separators and dot components without accessing the filesystem, following symlinks, or making
relative paths absolute.
Windows names and paths compare case-insensitively; POSIX comparisons are case-sensitive.

After filtering, ``prependPath(path)`` and ``appendPath(path)`` insert explicit additions.
Repeated prepends put the most recent addition first; appends retain call order.
An addition can restore a filtered path.
Finally, deduplication retains the first lexically equivalent entry and its original spelling.
Malformed inherited PATH entries are discarded when PATH edits run.
Unchanged environment values retain their original encoding.
The separator is ``;`` on Windows and ``:`` on POSIX.
Explicit path arguments must be valid and contain neither a null character nor the native PATH separator.
On Windows, paired outer quotes are accepted when reading entries, and ``%NAME%`` placeholders resolve once against the
environment being read or edited, using case-insensitive variable lookup.
For deferred changes, this is the resulting environment after inheritance and variable actions.
Unresolved or malformed placeholders, recursive expansions, embedded quotes, separator-containing expansions,
drive-relative paths such as ``C:tools``, and invalid native encoding are skipped.
Valid original quotes and placeholders are retained when writing other entries back.
Explicit Windows ``Path`` arguments must not contain quote or percent characters, which would introduce ambiguous PATH
syntax.

PATH processing runs only when a prepend, append, or removal rule is configured.
Default inheritance and raw PATH assignments preserve the original value exactly.
Empty PATH entries represent the current directory and are deduplicated with equivalent ``.`` entries.
Removing every entry leaves an existing PATH present with an empty value.
An absent PATH stays absent unless additions create it; ``removeVariable("PATH")`` can remove it completely, while PATH
additions can recreate it.
``clearPathChanges()`` preserves inheritance and variable actions.

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

Creation and Ownership
----------------------

``Subprocess::create(executable, arguments, options)`` prepares a shared ``SubprocessPtr`` without launching a child.
Install optional handlers, then call ``start()`` once.
Executables and arguments are passed directly without shell interpretation or PATH lookup.
Plain procedural use requires no event loop.
Releasing the last process owner requests termination, waits briefly, then forcefully terminates and reaps a running
child.
Event callbacks should use weak pointers or non-owning captures to avoid retaining their source indefinitely.

``DetachedSubprocess::start(...)`` launches independently and returns a shared ``DetachedSubprocessPtr`` exposing only
``processId()``.
It has no events, capture, wait or termination API.
Dropping this handle does not stop the child; an independent waiter prevents POSIX zombies.
Capture options are rejected for detached children.

``SubprocessOptions`` controls working directory, environment inheritance and changes, standard input inheritance,
output handling, capture limits and maximum output block size.
Its ``EnvironmentChanges`` descriptor selects inherited variables and applies variable and PATH changes at launch.

Waiting and Termination
-----------------------

``wait()`` returns native completion without running callbacks.
The timed overload returns no status when its non-negative deadline expires.
``isRunning()``, ``processId()``, and ``exitStatus()`` provide synchronized snapshots; the identifier remains valid
after exit.
Native launch, read and wait failures remain available through procedural exceptions.
Procedural waits and event observation can be mixed without consuming each other's completion state.

On POSIX, ``terminate()`` sends SIGTERM and ``kill()`` sends SIGKILL.
Windows uses forced native termination for both operations.
A process can be started only once, including after a failed launch.

Optional Owner-Loop Events
--------------------------

Prefer creating event-driven processes through the loop's system frontend:

.. code-block:: cpp

    auto process = events->get<el::System>().createSubprocess(executable, arguments, options);
    process->events().onExit([](el::ExitCode code) -> void {
        // Inspect an ordinary zero or nonzero exit.
    }).onFinal([]() -> void {
        // The workflow has ended.
    });
    process->start();

The frontend is obtained as a reference from ``Events::get<System>()``.
Creation through it and handler editing must occur on its owner loop.
For manually created processes, the first ``events()`` call binds the current loop, or throws ``LogicFailure`` if the
thread has no active loop.
Binding cannot later change; past lifecycle events are not replayed.
The source owns one stable ``SubprocessEventEditor`` with the usual ``source()`` and ``target()`` accessors.

``onStarted()`` reports successful native launch.
``onStandardOutput()`` and ``onStandardError()`` deliver ``String`` blocks with the initial UTF-8 BOM removed unless
removal was disabled in the launch options.
Pending output is delivered before one terminal outcome: ``onExit(ExitCode)`` for an ordinary exit, including nonzero
codes, or ``onError(const PlatformErrorContext &)`` for launch, read, wait or crash failures.
``onFinal()`` follows either outcome, including when launch failed.
Fatal launch exceptions, including allocation and thread-creation failures, propagate unchanged from ``start()`` after
cleanup.
They do not emit ``onError()`` or ``onFinal()``.
POSIX signal termination is abnormal; Windows exit statuses with the high bit set are treated as exception-style
outcomes.
Windows exposes an exit code rather than definitive proof that the child crashed.

All callbacks execute on the owner loop and callback exceptions follow that loop's configured error policy.
Blocking waits neither pump the loop nor wait for callback delivery.
Releasing the source suppresses callbacks that have not begun dispatching.

Standard Streams and Buffering
------------------------------

Stdout and stderr independently use inheritance, discard or bounded capture.
Merged stderr follows the stdout destination and does not invoke the stderr handler.
Standard input is inherited or connected to an immediately closed null stream.

For a Capture stream without an output handler, Core retains a prefix up to ``captureLimit()`` and drains excess bytes.
Setting its handler switches subsequent bytes to event delivery and disables further capture for that stream.
Clearing the handler discards queued event bytes and resumes capture, preserving earlier captured bytes.
Replacing a handler affects pending delivery.

By default, Core removes one initial UTF-8 BOM from each captured stream before retaining or delivering output.
Set ``SubprocessOptions::setRemoveOutputBom(false)`` to preserve it when reading byte-exact machine output;
``removesOutputBom()`` reports the selected policy.
The signature may span native pipe reads and does not count toward the capture limit or pending-byte budget.
Merged stdout and stderr form one stream, so only its initial signature is removed.
Changing output handlers does not restart BOM handling.

``maximumOutputChunkSize()`` bounds a callback's byte count, defaults to 4096 and must be finite, representable and at
least 256. Valid UTF-8 code points are not split between callbacks.
All remaining bytes, including malformed bytes and later U+FEFF sequences, are preserved without decoding or
replacement; an unfinished trailing sequence or incomplete initial BOM is emitted unchanged at EOF.
Blocks are not line-oriented and may contain invalid UTF-8 or null bytes.

Each event-driven stream uses ``captureLimit()`` as its own pending-byte budget.
When the loop cannot keep up, new excess bytes are silently discarded while native pipes continue draining.
The stream's truncation flag is set and ``onOutputTruncated()`` runs once per process if registered.
Overflow is not an operational error.
Subsequent output can be accepted after the buffer drains.
One coalesced dispatch per process bounds queued delivery work.


Environment-variable throwing operations report invalid names or values as ``ParseError`` and absent variables as
``NotFoundError``.
Native operation failures remain ``PlatformError``.

Interface
=========

.. doxygenclass:: erbsland::system::CpuArchitecture
    :members:
.. doxygenclass:: erbsland::system::DetachedSubprocess
    :members:

.. doxygentypedef:: erbsland::system::DetachedSubprocessPtr
.. doxygenclass:: erbsland::system::EnvironmentChanges
    :members:
.. doxygenenum:: erbsland::system::EnvironmentInherit
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

.. doxygentypedef:: erbsland::system::SubprocessPtr
.. doxygenclass:: erbsland::system::SubprocessEventEditor
    :members:
.. doxygenclass:: erbsland::system::SubprocessExitStatus
    :members:
.. doxygenclass:: erbsland::system::SubprocessOptions
    :members:
.. doxygenenum:: erbsland::system::SubprocessOutputMode
.. doxygenclass:: erbsland::system::System
    :members:
.. doxygenfunction:: erbsland::system::info::operatingSystem() noexcept -> OperatingSystem

.. doxygenfunction:: erbsland::system::info::cpuArchitecture() noexcept -> CpuArchitecture

.. doxygenfunction:: erbsland::system::info::logicalCpuCount() noexcept -> std::uint32_t
.. doxygenclass:: erbsland::system::UserId
    :members:
.. doxygenclass:: erbsland::system::UserLookup
    :members:
.. doxygenclass:: erbsland::system::UserName
    :members:
