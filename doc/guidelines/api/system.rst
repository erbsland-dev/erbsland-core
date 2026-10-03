****************************
System Domain API Guidelines
****************************

Core Semantics
==============

.. code-block:: text

    user or group id = platform identity identifier such as a POSIX UID/GID or Windows SID string
    user or group name = display name resolved from a platform identity identifier, with optional domain
    environment variable = process-wide native name/value entry
    process id = opaque potentially valid identifier that need not identify an existing process
    process identity = process id plus valid start time when identifier reuse matters
    subprocess = one directly launched operating-system child, never a shell command

Primary Types
=============

.. code-block:: text

    UserId, GroupId // type-safe platform identity identifiers
    UserName, GroupName // type-safe platform identity names
    UserLookup // cached lookup service for platform user and group identities
    EnvironmentVariables // portable access to process environment variables
    EnvironmentChanges // deferred inheritance, variable changes, and PATH edits
    ProcessId // opaque process identifier created only by system APIs
    ProcessInfo // eager portable process snapshot
    System // event-loop frontend for system event sources
    Subprocess, SubprocessPtr // shared owner and controller for one child process
    DetachedSubprocess, DetachedSubprocessPtr // independent launch identifier
    SubprocessEventEditor // owner-loop handler replacement
    SubprocessOptions // launch environment, working directory, and stream policies
    SubprocessExitStatus // exit code or POSIX termination signal

Secondary Types
===============

.. code-block:: text

    PlatformError // native operating-system failure
    PlatformErrorContext // portable structured native failure context
    PlatformErrorCategory // native subsystem or API failure category
    OperatingSystem, CpuArchitecture // comparable smart-enum host classifications
    SubprocessOutputMode // inherit, discard, or bounded capture
    EnvironmentInherit // all or no base environment variables

Lookup Patterns
===============

.. code-block:: text

    o.userNameForId(id) -> UserName // resolve a platform user identifier to a display name
    o.groupNameForId(id) -> GroupName // resolve a platform group identifier to a display name
    o.userIdForName(name) -> UserId // resolve a platform user name to an identifier
    o.groupIdForName(name) -> GroupId // resolve a platform group name to an identifier
    o.clearCache() // clear cached identity lookups

Environment Variable Patterns
=============================

.. code-block:: text

    o.get(name) -> optional<String> // read while preserving missing versus empty
    o.get(name, defaultValue) -> String // read with an explicit fallback
    o.getOrThrow(name) -> String // read or throw if missing or inaccessible
    o.set(name, value) -> bool // set without throwing
    o.setOrThrow(name, value) // set or throw on failure
    o.remove(name) -> bool // idempotently remove without throwing
    o.removeOrThrow(name) // idempotently remove or throw on failure

PATH Access Patterns
====================

.. code-block:: text

    o.paths()/pathsOrThrow() -> PathList // valid normalized PATH entries in search order
    o.prependPath(path)/appendPath(path) -> bool // edit PATH, dropping malformed entries
    o.insertPath(index, path) -> bool // insert at a position in the deduplicated list
    o.removePath(path)/removePathsIn(base) -> bool // remove equivalent entries or descendants
    o.❮pathEdit❯OrThrow(...) // preserve invalid-argument and native failure details
    o.executableCandidates(name)/executableCandidatesOrThrow(name) -> PathList // candidates without filesystem checks

Environment Change Patterns
===========================

.. code-block:: text

    o.setInherit(EnvironmentInherit) // select all or no base variables
    o.setInherit(StringList) // replace the selection of inherited variable names
    o.isUnchangedInheritance() -> bool // test for unmodified full inheritance
    o.setVariable(name, value) // replace a pending assignment or removal
    o.removeVariable(name/StringList) // remove after inheritance
    o.clearVariableChange(name) // restore inheritance behavior for one name
    o.clearVariableChanges() // clear assignments and removals only
    o.prependPath(path)/appendPath(path) // insert at the front/back after filtering
    o.removePath(path) // remove lexically equivalent PATH entries
    o.removePathsIn(base) // remove the base directory and its descendants
    o.clearPathChanges() // clear PATH rules only
    o.apply(base) -> StringMap<String> // return the transformed environment without process access

Process Information Patterns
============================

.. code-block:: text

    T() // eagerly load the current-process snapshot
    T(processId) // eagerly load the process currently using the identifier
    o.processId()/exists() -> T // inspect the followed identifier and cached existence
    o.executablePath/parentProcessId/startTime/ownerId() -> T // inspect cached values or invalid sentinels
    o.❮attribute❯OrThrow() -> T // inspect a cached value or rethrow its cached native diagnostic
    o.reload() // atomically replace the snapshot without throwing
    o.reloadOrThrow() // atomically replace the snapshot and report an unexpected lookup failure

System Information Patterns
===========================

.. code-block:: text

    operatingSystem() -> OperatingSystem // classify the current host in system::info
    cpuArchitecture() -> CpuArchitecture // classify the native host architecture in system::info
    logicalCpuCount() -> uint32_t // get a usable thread-count hint of at least one in system::info

Subprocess Patterns
===================

.. code-block:: text

    T::create(executable, arguments, options) -> SubprocessPtr // prepare without launching
    o.start() // launch the prepared child once
    T::start(executable, arguments, options) -> DetachedSubprocessPtr // independent detached launch
    o.get<System>() -> System& // access the loop frontend
    o.createSubprocess(executable, arguments, options) -> SubprocessPtr // prepare a bound source
    o.events() -> SubprocessEventEditor& // lazily bind manual sources to the current loop
    o.on❮Event❯(callback) -> SubprocessEventEditor& // replace a handler on the owner loop
    o.processId() -> ProcessId // inspect the launched identifier, including after exit
    o.isRunning() -> bool // poll native child state
    o.exitStatus() -> optional<SubprocessExitStatus> // inspect cached terminal state
    o.wait() -> SubprocessExitStatus // wait indefinitely and reap
    o.wait(timeout) -> optional<SubprocessExitStatus> // wait until a deadline and reap on exit
    o.terminate()/kill() // request regular or forceful native termination
    o.standardOutput()/standardError() -> String // read a retained captured prefix
    o.was❮Stream❯Truncated() -> bool // detect output beyond the capture limit
    o.environmentChanges() -> EnvironmentChanges& // edit the launch-time environment descriptor
    o.setEnvironmentChanges(changes) -> SubprocessOptions& // replace the descriptor
    o.removesOutputBom() -> bool // initial output signature removal policy
    o.setRemoveOutputBom(enabled) -> SubprocessOptions& // preserve machine output bytes when disabled

Application Patterns
====================

.. code-block:: text

    application().userLookup() -> UserLookup& // access the application-shared lookup service

Platform Error Patterns
=======================

.. code-block:: text

    T(operation[, nativeCode]) // create native failure context for an operation
    o.category()/nativeCode()/nativeMessage() -> T // inspect structured native failure data
    o.set❮Property❯(value) -> PlatformErrorContext& // fluently add portable diagnostic context
