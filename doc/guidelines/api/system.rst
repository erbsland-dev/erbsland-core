****************************
System Domain API Guidelines
****************************

Core Semantics
==============

Identity Lookup
---------------

.. code-block:: text

    user id = platform owner identifier such as a POSIX UID or Windows SID string
    group id = platform group identifier such as a POSIX GID or Windows SID string
    user name = display name resolved from a platform user id, with optional domain
    group name = display name resolved from a platform group id, with optional domain

Environment Variables
---------------------

.. code-block:: text

    environment variable = process-wide native name/value entry
    missing variable != variable with an empty value
    variable names follow native case-sensitivity rules
    remove of a missing variable = success

Subprocesses
------------

.. code-block:: text

    subprocess = one directly launched operating-system child, never a shell command
    owned subprocess destruction = terminate, bounded wait, force termination, reap
    detached subprocess = explicit launch-and-forget with internal POSIX reaping
    captured output = bounded retained prefix while the native pipe continues to drain

Primary Types
=============

.. code-block:: text

    UserId, GroupId // type-safe platform identity identifiers
    UserName, GroupName // type-safe platform identity names
    UserLookup // cached lookup service for platform user and group identities
    EnvironmentVariables // portable access to process environment variables
    Subprocess // move-only owner and controller for one child process
    SubprocessOptions // launch environment, working directory, and stream policies
    SubprocessExitStatus // exit code or POSIX termination signal

Secondary Types
===============

.. code-block:: text

    PlatformError // native operating-system failure
    PlatformErrorContext // portable structured native failure context
    PosixErrorContext, WindowsErrorContext // platform-specific diagnostic contexts
    PlatformErrorCategory // native subsystem or API failure category
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

Subprocess Patterns
===================

.. code-block:: text

    T::start(executable, arguments, options) -> Subprocess // launch an owned direct child
    T::startDetached(executable, arguments, options) // launch without process control or capture
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
