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

Primary Types
=============

.. code-block:: text

    UserId, GroupId // type-safe platform identity identifiers
    UserName, GroupName // type-safe platform identity names
    UserLookup // cached lookup service for platform user and group identities

Secondary Types
===============

.. code-block:: text

    PlatformError // native operating-system failure
    PlatformErrorContext // portable structured native failure context
    PosixErrorContext, WindowsErrorContext // platform-specific diagnostic contexts
    PlatformErrorCategory // native subsystem or API failure category

Lookup Patterns
===============

.. code-block:: text

    o.userNameForId(id) -> UserName // resolve a platform user identifier to a display name
    o.groupNameForId(id) -> GroupName // resolve a platform group identifier to a display name
    o.userIdForName(name) -> UserId // resolve a platform user name to an identifier
    o.groupIdForName(name) -> GroupId // resolve a platform group name to an identifier
    o.clearCache() // clear cached identity lookups

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
