****************************
System Domain API Guidelines
****************************

These guidelines extend the Common API Guidelines for public APIs that expose operating-system services.

The purpose of this document is to define a base naming vocabulary for system APIs.
It is intentionally plain, technical, and list-based to provide a quick overview of method names and their usage
patterns.
If new vocabulary is introduced, update this page to provide a reference for future extensions.

Core Semantics
==============

Identity Lookup
---------------

.. code-block:: text

    user id // platform owner identifier such as a POSIX UID or Windows SID string
    group id // platform group identifier such as a POSIX GID or Windows SID string
    user name // display name resolved from a platform user id, with optional domain
    group name // display name resolved from a platform group id, with optional domain

Primary Types
=============

.. code-block:: text

    UserId, GroupId // type-safe platform identity identifiers
    UserName, GroupName // type-safe platform identity names
    UserLookup // cached lookup service for platform user and group identities

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
