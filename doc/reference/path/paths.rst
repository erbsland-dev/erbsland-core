.. index::
    single: File System Paths

*****************
File System Paths
*****************

Introduction
============

The path module provides ``Path`` as the platform-independent value type for filesystem path text and exposes components
for cached metadata, file content, directory traversal, filesystem operations, and managed temporary resources.
It can parse generic, POSIX, and Windows path forms, inspect path elements, edit file names and suffixes, join and slice
paths, convert external formats, and perform portable filesystem work with structured diagnostics.

For the domain overview and filesystem behavior, see :doc:`/topics/path/overview`.
For practical path-value examples, see :doc:`/topics/path/working_with_paths`.

Path Diagnostics
================

``PathError`` contains a ``PathErrorContext`` that describes the failed operation with a trusted title, description, and
optional explicit help.
It can carry source and target paths plus an immutable platform context without creating a duplicate nested error.
Source-only diagnostics render ``path``; copy, move, and rename diagnostics can render ``source path`` and
``target path``.
Path fields appear below a ``Paths`` section, while native codes and messages appear below ``Platform Error``.
Keeping the two groups explicit makes their independently aligned labels unambiguous and separates user-correctable
operation values from platform-specific implementation details.

Paths and native messages cross the application trust boundary and are escaped for display.
Path separators remain semantic separator nodes so terminal renderers can wrap long paths naturally.
Explicit context help takes precedence over concise remedies derived from the portable platform category.
A separate exception cause remains available for a genuinely independent failure layer.

Interface
=========

.. doxygenclass:: erbsland::path::Path
    :members:
.. doxygenclass:: erbsland::path::PathAccessInfo
    :members:
.. doxygenenum:: erbsland::path::PathAccessProfile
.. doxygenenum:: erbsland::path::PathAccessRight

.. doxygentypedef:: erbsland::path::PathAccessRights
.. doxygenenum:: erbsland::path::PathAttribute

.. doxygentypedef:: erbsland::path::PathAttributes
.. doxygenclass:: erbsland::path::PathChangeOptions
    :members:
.. doxygenenum:: erbsland::path::PathCollisionMode
.. doxygenclass:: erbsland::path::PathContent
    :members:
.. doxygenclass:: erbsland::path::PathCopyOptions
    :members:
.. doxygenclass:: erbsland::path::PathCreateDirectoryOptions
    :members:
.. doxygenclass:: erbsland::path::PathCreateFileOptions
    :members:
.. doxygenenum:: erbsland::path::PathCreateMode
.. doxygenclass:: erbsland::path::PathError
    :members:
.. doxygenclass:: erbsland::path::PathErrorContext
    :members:
.. doxygenenum:: erbsland::path::PathFormat
.. doxygenclass:: erbsland::path::PathInfo
    :members:
.. doxygenenum:: erbsland::path::PathInfoPart

.. doxygentypedef:: erbsland::path::PathInfoParts
.. doxygenclass:: erbsland::path::PathMoveOptions
    :members:
.. doxygenclass:: erbsland::path::PathOperations
    :members:
.. doxygenenum:: erbsland::path::PathProgressStatus

.. doxygenstruct:: erbsland::path::PathProgress
    :members:
.. doxygenclass:: erbsland::path::PathReadDataOptions
    :members:
.. doxygenclass:: erbsland::path::PathReadTextOptions
    :members:
.. doxygenclass:: erbsland::path::PathRemoveOptions
    :members:
.. doxygenenum:: erbsland::path::PathResolveMode
.. doxygenclass:: erbsland::path::PathResolveOptions
    :members:
.. doxygenclass:: erbsland::path::PathTempDirectoryOptions
    :members:
.. doxygenclass:: erbsland::path::PathTempFileOptions
    :members:
.. doxygenenum:: erbsland::path::PathType
.. doxygenenum:: erbsland::path::PathWalkDirection
.. doxygenclass:: erbsland::path::PathWalker
    :members:
.. doxygentypedef:: erbsland::path::PathWalkFn

.. doxygentypedef:: erbsland::path::PathInfoWalkFn
.. doxygenclass:: erbsland::path::PathWalkOptions
    :members:
.. doxygenclass:: erbsland::path::PathWalkResult
    :members:
.. doxygenenum:: erbsland::path::PathWalkStatus
.. doxygenenum:: erbsland::path::PathWindowsFormat
.. doxygenclass:: erbsland::path::PathWriteDataOptions
    :members:
.. doxygenclass:: erbsland::path::PathWriteTextOptions
    :members:
.. doxygenenum:: erbsland::path::SymlinkMode
.. doxygenclass:: erbsland::path::TempDirectory
    :members:
