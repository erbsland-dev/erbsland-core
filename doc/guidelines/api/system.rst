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
    ProcessId // opaque process identifier created only by system APIs
    ProcessInfo // eager portable process snapshot
    Subprocess // move-only owner and controller for one child process
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

    T::start(executable, arguments, options) -> Subprocess // launch an owned direct child
    T::startDetached(executable, arguments, options) // launch without process control or capture
    o.processId() -> ProcessId // inspect the launched identifier, including after exit
    o.isRunning() -> bool // poll native child state
    o.exitStatus() -> optional<SubprocessExitStatus> // inspect cached terminal state
    o.wait() -> SubprocessExitStatus // wait indefinitely and reap
    o.wait(timeout) -> optional<SubprocessExitStatus> // wait until a deadline and reap on exit
    o.terminate()/kill() // request regular or forceful native termination
    o.standardOutput()/standardError() -> String // read a retained captured prefix
    o.was❮Stream❯Truncated() -> bool // detect output beyond the capture limit
    o.setEnvironmentVariable(name, value) // assign after optional inheritance
    o.removeEnvironmentVariable(name) // remove after optional inheritance

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
