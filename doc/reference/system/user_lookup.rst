.. index::
    single: User Lookup

***********
User Lookup
***********

Introduction
============

The ``system`` namespace provides application-shared services that expose small, portable wrappers around native
operating-system facilities.

Native failures use immutable ``PlatformErrorContext`` values.
The POSIX and Windows implementations capture the native code and message immediately and expose a portable
``PlatformErrorCategory`` for domain diagnostics and actionable help.
``PlatformError`` is the concrete exception used when the native failure itself is the reported error.

``UserLookup`` resolves owner and group identifiers into display names and resolves names back to platform identifiers.
It caches successful lookups and is available through
:cpp:func:`Application::userLookup() <erbsland::core::Application::userLookup>` for code that needs a shared service
instance.

Interface
=========

.. doxygenclass:: erbsland::system::GroupId
    :members:
.. doxygenclass:: erbsland::system::GroupName
    :members:
.. doxygenclass:: erbsland::system::PlatformError
    :members:
.. doxygenclass:: erbsland::system::PlatformErrorCategory
    :members:
.. doxygenclass:: erbsland::system::PlatformErrorContext
    :members:
.. doxygenclass:: erbsland::system::PosixErrorContext
    :members:
.. doxygenclass:: erbsland::system::UserId
    :members:
.. doxygenclass:: erbsland::system::UserLookup
    :members:
.. doxygenclass:: erbsland::system::UserName
    :members:
.. doxygenclass:: erbsland::system::WindowsErrorContext
    :members:
