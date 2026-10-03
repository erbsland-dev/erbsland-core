..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: File-System Paths; Reference
    single: File System Paths

*****************
File-System Paths
*****************

Introduction
============

The path module provides ``Path`` as the platform-independent value type for filesystem path text and exposes components
for cached metadata, file content, directory traversal, filesystem operations, and managed temporary resources.
It can parse generic, POSIX, and Windows path forms, inspect path elements, edit file names and suffixes, join and slice
paths, convert external formats, and perform portable filesystem work with structured diagnostics.

For the domain overview and filesystem behavior, see :doc:`/topics/path/overview`.
For practical path-value examples, see :doc:`/topics/path/working_with_paths`.

Path Equality and Ordering
==========================

``Path::compare()`` and the comparison operators compare stored roots and then individual path components.
They allocate no temporary strings or lists and do not access the filesystem.
Empty paths sort first, an absent root sorts before a present root, and an equal component prefix sorts before its
descendants.
Within each root or component, the default comparison uses decoded, case-sensitive code-point order.
An optional character comparator changes that character ordering without changing component boundaries.

Component ordering differs from serialized-text ordering: ``a/b`` precedes ``a-b``, because the component ``a`` is a
prefix of ``a-b``.
Format metadata and cached filesystem information do not affect comparison.
Dot components remain lexical values: comparing ``a/./b`` with ``a/b`` does not resolve either path.
Equal path values need not identify the same physical file, and different values may identify the same file through
links or filesystem case rules.
See :doc:`/topics/path/working_with_paths` for choosing value comparison versus filesystem operations.

Joining Paths
=============

``join()`` modifies the path; ``joined()`` returns a joined copy.
Their string overloads and the ``/`` and ``/=`` operators parse text using the generic ``Path{text}`` constructor.
In relative generic text, both slash and backslash are separators, including on POSIX systems.

Use ``joinNative()`` or ``joinedNative()`` for native filenames and path text.
These methods parse the argument with ``Path::fromNative()`` before joining.
For example, ``base.joinedNative(R"(a\b.txt)"_el)`` appends one filename containing a literal backslash on POSIX and two
components on Windows.

All joining forms preserve the base path's root and discard the appended path's root.
An empty base remains empty, and an argument that parses to an empty path leaves the base unchanged.
Joining neither accesses the filesystem nor resolves dot components.
Use the throwing native parser explicitly when malformed argument text must be reported.

Native Argument Text
====================

``Path::toString()`` exports portable path text, while ``Path::toNativeString()`` exports text for native command-line
arguments.
The native form uses POSIX spelling on POSIX hosts and backslashes with ``PathWindowsFormat::Native`` on Windows.
It adds no shell quoting or escaping; pass the result as one ``Subprocess`` argument.
As with the explicit format conversions, a path that cannot be represented in the selected native format produces an
empty string.

Process Directories
===================

``Path::currentDirectory()`` returns the process working directory.
``Path::executablePath()`` returns the absolute executable image path of the current process and is a convenience
wrapper around :cpp:class:`erbsland::system::ProcessInfo <erbsland::system::ProcessInfo>`.
Its non-throwing form returns an empty path on failure, while ``Path::executablePathOrThrow()`` translates the native
system diagnostic into a ``PathError`` without discarding its native context.
``Path::userHomeDirectory()`` resolves the effective user's configured home or profile directory through the native
account system rather than an environment variable.
It returns an absolute native path without creating or checking the directory.
The ``OrThrow`` form reports lookup and conversion failures as ``PathError``; the non-throwing form returns an empty
path.
``Path::systemTempDirectory()`` resolves the platform temporary directory.

File Locks
==========

``Path::createLock()`` acquires a nonblocking exclusive process lock and returns a move-only ``FileLock`` owner.
The operating-system lock is held on a persistent ``.lock`` sidecar file, so you can replace the protected path
atomically without dropping mutual exclusion.
The sidecar remains after release to avoid races between lock users, and existing sidecar content is preserved.
Acquisition reports an invalid path, an inaccessible sidecar, or lock contention as a
:cpp:class:`PathError <erbsland::path::PathError>`.

Path Information Cache
======================

Each non-empty ``Path`` lazily owns one path-information cache, and ordinary copies of that path share it.
Lexical normalization also shares that cache when the path has no dot components to normalize.
Path conversions with an explicit absolute base do not query the process working directory.
Repeated ``Path::info()`` calls during the one-second cache period therefore reuse previously loaded metadata and the
resolved physical path.
Requesting another information part can extend the snapshot without resolving the path again while the cached resolution
is current.
``PathInfo::reload()`` explicitly resolves and refreshes the path.

Directory traversal seeds each returned child path with metadata obtained by the native directory enumeration.
On POSIX systems this includes the entry type when the filesystem supplies it.
On Windows it also includes the size, timestamps, access approximation, and native attributes returned by
``FindFirstFileExW`` and ``FindNextFileW``.
The walker trusts each enumeration snapshot for the duration of that walk, even when processing a very large sibling set
takes longer than the normal cache period.

Successful mutations through the library invalidate the cache attached to each directly affected path.
External filesystem changes remain snapshot-based and become visible after cache expiry or an explicit reload.

Requesting ``PathInfoPart::FileIdentity`` loads an opaque, comparable identity for a regular filesystem object.
POSIX backends derive it from device and inode information; Windows backends use volume serial and file index data.
The identity is invalid when the path does not resolve to an identifiable object.

Filtering a Directory Walk
==========================

``PathWalkOptions::setFilter()`` selects reported entries from their path and effective type before the walker requests
additional ``infoParts()``.
An empty filter accepts all entries selected by ``types()``.
Rejected directories are still traversed.
``setDescendFilter()`` independently selects which directories to enter, including the starting directory, and applies
before descent in either walk direction.
A pruned directory can still be reported.

``setGlobPattern()`` adds a compiled :doc:`path_glob_patterns` filter, or compiles a supplied string and flags.
It matches relative to the starting directory, which has zero relative components.
Full matching is the default: ``*.cpp`` selects immediate children, while ``**/*.cpp`` includes deeper files.
``setGlobMatchMode(PathGlobMatchMode::Suffix)`` selects suffix matching instead.

Glob matching precedes the reporting filter and requested metadata.
Descendant feasibility precedes the descent filter.
Nonmatching ancestors remain traversable, while impossible branches reach neither callback.
A null compiled pattern disables glob filtering.
A compilation error leaves the previous option intact.

Both filters use ``PathWalkFilterFn``.
Symlink handling precedes filtering, so skipped links never reach either filter; followed links have their target type.
Filters also apply to the starting path.
They run synchronously and use the same exception handling as the walk callback.
Name predicates can select extensions without requesting file sizes, ownership, or timestamps for rejected entries.
Native enumeration and deterministic sorting still occur for visited directories.

Reading Content
===============

``Path::content()`` opens byte and text input streams and provides bounded whole-file helpers on top of those streams.
Input options default to ``SymlinkMode::Follow`` for compatibility with ordinary filesystem reads.
Select ``Skip`` or ``Use`` to reject symbolic links and reparse points instead.
Restrictive POSIX opens walk path components with ``openat`` and ``O_NOFOLLOW``; Windows opens inspect every component
with reparse-point processing disabled.
Text reads propagate the selected policy to their underlying byte stream.

Replacing Content and Retaining Backups
=======================================

``PathContent::replaceTextOrThrow()`` and ``replaceDataOrThrow()`` write complete content into an exclusively created
sibling temporary file.
They finalize the output before replacing the destination with a native same-filesystem move.
The destination can be absent or a regular file; symbolic links, reparse points, and other file types are rejected.
There is no fallback that deletes the live destination or copies new content into it.
The non-throwing forms return ``util::Result`` and intentionally discard diagnostic details.

.. code-block:: cpp

    auto options = el::PathReplaceTextOptions{}.setBackupCount(el::ItemCount{3U});
    destination.content().replaceTextOrThrow(newText, options);

``PathReplaceDataOptions`` and ``PathReplaceTextOptions`` provide output settings, a new-file access profile, and
forwarding setters for ``PathCommitOptions``.
Text options also select the encoding and byte-order-mark policy.
The default timeout is 60 seconds per stream operation.
Parent creation and backups are disabled by default.
Replacement uses the temporary file's metadata; it does not preserve destination ownership, permissions, ACLs, or
hard-link relationships.
Flushing and native replacement do not promise portable crash durability.

``backupCount`` retains that many overwritten versions beside the destination, excluding the current file.
For ``report.txt``, the newest backup is ``report.txt.bak.1``, followed by ``report.txt.bak.2`` and so on.
Enabling backups reserves this naming scheme.
Only canonical positive decimal generations representable as native size values are managed; names such as
``report.txt.bak.01`` remain untouched.
Reducing the count removes excess generations.
Setting it to zero leaves all existing backups untouched.
A new destination does not create a backup.

Backup copies use the ``UserOnly`` access profile to avoid exposing old content through broader default permissions.
The old destination is copied through bounded byte transfers into a second sibling temporary file and finalized before
backup rotation begins.
Managed backup entries must be regular files, never links or reparse points.
The live destination remains in place until the final replacement move.
By default, a backup failure stops replacement.
``setIgnoreBackupErrors(true)`` makes backup preparation best effort: it suppresses backup failures and still attempts
replacement.
Destination validation, caller checks, and replacement failures are never suppressed by this setting.

Backup rotation and destination replacement are separate filesystem operations.
A failed attempt may partly rotate backups or install a backup without replacing the destination.
No rollback of backup history is promised.
Collision checks and path validation are best effort under concurrent filesystem changes.
Callers that need coordination can use the existing sidecar file lock around the entire operation.

Committing Temporary Output
===========================

``TempByteOutputStream::commitToOrThrow()`` and ``TempTextOutputStream::commitToOrThrow()`` use the same replacement
engine, accepting ``PathCommitOptions``.
Their non-throwing ``commitTo()`` forms return ``util::Result``.
A commit flushes and closes the backing stream without releasing its temporary file, then prepares requested backups and
moves the temporary file into place.
Only successful replacement releases cleanup ownership.
Failed commits preserve the caller's cleanup policy and support retry after finalization, including a close timeout.
An empty or released temporary lease cannot be committed.
Destruction never commits and never waits for cleanup.

The collision policy defaults to ``Overwrite``; ``Stop`` is also supported, while ``Skip`` is rejected.
An optional ``beforeCommit`` callback receives the absolute temporary and destination paths.
It runs after finalization, before backup mutations, and again before the destination move when backup preparation was
attempted.
It must tolerate repeated calls and must not modify the temporary stream.
A callback must report recoverable failures with ``err::RuntimeError`` or a derived type.
These failures cancel the commit and are retained as diagnostic causes.
Unexpected exceptions, including logic errors and allocation failures, propagate unchanged; they are never converted
into replacement errors or ignored backup failures.
The non-throwing status methods terminate on unexpected exceptions because they are ``noexcept``.

Each failure reports its stage and relevant source and destination paths.
Underlying stream exceptions, nested path exceptions, and native error details are preserved as causes.
Render the complete chain with ``err::DiagnosticHelper`` instead of displaying only ``what()``.
Cleanup is best effort and never masks the original failure or claims that scheduled removal has already completed.

Portable Metadata Changes
=========================

``PathOperations::setLastModified()`` changes a file or directory modification time through the native filesystem
backend.
The operation accepts ``PathChangeOptions``, including its recursive and symbolic-link policy, invalidates cached path
information after a successful change, and reports unrepresentable timestamps or native failures as ``PathError``.
The throwing ``setLastModifiedOrThrow()`` variant is suitable when preserving archive or copied-file timestamps is part
of the operation's correctness contract.

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

.. doxygenclass:: erbsland::path::FileLock
    :members:
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
.. doxygentypedef:: erbsland::path::PathCommitCheckFn
.. doxygenclass:: erbsland::path::PathCommitOptions
    :members:
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
.. doxygenstruct:: erbsland::path::PathProgress
    :members:
.. doxygenenum:: erbsland::path::PathProgressStatus
.. doxygenclass:: erbsland::path::PathReadDataOptions
    :members:
.. doxygenclass:: erbsland::path::PathReadTextOptions
    :members:
.. doxygenclass:: erbsland::path::PathRemoveOptions
    :members:
.. doxygenclass:: erbsland::path::PathReplaceDataOptions
    :members:
.. doxygenclass:: erbsland::path::PathReplaceTextOptions
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

.. doxygentypedef:: erbsland::path::PathWalkFilterFn
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
