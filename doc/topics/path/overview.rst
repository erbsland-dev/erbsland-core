..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************
Path API Overview
*****************

The path domain combines a platform-independent path value with explicit components for filesystem information, content,
traversal, and mutation.
A :cpp:class:`Path <erbsland::path::Path>` does not access the filesystem merely by being constructed; it stores a
normalized logical path and becomes the starting point for the other components.

Choosing a Component
====================

Use :cpp:func:`Path::info() <erbsland::path::Path::info>` for cached information about an existing filesystem object,
including its type, size, timestamps, access rights, owner, group, and native attributes.

Use :cpp:func:`Path::content() <erbsland::path::Path::content>` to read or write a regular file as text, byte data, or
an open stream.
Creation modes distinguish creating a new file, overwriting one, and appending to one.

Use :cpp:func:`Path::walker() <erbsland::path::Path::walker>` to visit a complete tree.
The base path is included in the walk.
Root-to-leaf walks report a directory before its contents, while leaf-to-root walks report it after all its contents.
Type filters only control callbacks; directories are still traversed when directory reporting is disabled.

Use :cpp:func:`Path::operations() <erbsland::path::Path::operations>` for creating, copying, moving, removing, and
changing metadata.
Recursive operations use the same traversal rules as ``PathWalker`` so symlink and ignored-error behavior remains
consistent.

Errors and Non-Throwing Forms
=============================

Operations that can fail generally have a throwing form ending in ``OrThrow`` and a compact non-throwing form.
The throwing form preserves a :cpp:class:`PathError <erbsland::path::PathError>` with source and destination paths plus
the native platform context.
Filesystem access uses the native POSIX or Windows API rather than ``std::filesystem`` so a failed operating-system call
retains its native error code, message, and portable category.
The ``std::filesystem::path`` API on ``Path`` is an interoperability conversion only and performs no filesystem access.
The non-throwing form returns an empty value, ``std::nullopt``, or a failed
:cpp:class:`Result <erbsland::util::Result>`, depending on the operation.
These forms convert exceptions from the Erbsland exception hierarchy; unexpected foreign exceptions are not silently
treated as ordinary filesystem failures.

Traversal and Symlinks
======================

:cpp:enum:`SymlinkMode <erbsland::path::SymlinkMode>` has three explicit policies.  ``Skip`` neither reports nor follows
a symbolic link.
``Use`` reports the link itself and never descends through it.
``Follow`` reports information about the target and descends into directory targets.
Physical directory paths are tracked during followed walks so a link back to an already visited directory cannot create
an infinite traversal cycle.

A callback can continue, skip a directory subtree, stop successfully, or report failure.
Skipping prevents descent during root-to-leaf traversal.
During leaf-to-root traversal, children have already been visited when the callback runs, so skipping is equivalent to
continuing.
With ignored errors enabled, filesystem failures do not stop remaining entries, but the final walk result still reports
failure.

Copying, Moving, and Removing
=============================

Copy and move destinations identify the exact target path; an existing directory is not treated as an implicit parent.
Collision mode can stop, skip the complete operation, or replace the existing destination tree.
Source and destination trees may not overlap.
Move uses a native rename and therefore requires both paths to reside on the same filesystem.

Recursive removal is leaf-to-root.
``keepBase`` removes the contents but retains the starting directory.
Removing a filesystem root is rejected.
A prescan can provide an exact :cpp:struct:`PathProgress <erbsland::path::PathProgress>` total; without one, the total
is the infinite ``ItemCount`` value, which represents an unknown count.

Temporary Resources
===================

:cpp:func:`Path::userHomeDirectory() <erbsland::path::Path::userHomeDirectory>` returns the effective user's home
directory as an absolute native path.
It queries the operating-system account or profile database and deliberately ignores environment variables such as
``HOME``.
The lookup does not create the directory and does not require it to exist.
Use :cpp:func:`Path::userHomeDirectoryOrThrow() <erbsland::path::Path::userHomeDirectoryOrThrow>` when failure needs a
structured :cpp:class:`PathError <erbsland::path::PathError>`; the non-throwing form returns an empty path.

:cpp:func:`Path::systemTempDirectory() <erbsland::path::Path::systemTempDirectory>` returns the platform temporary
directory.
On POSIX systems ``TMPDIR`` is used when it names a usable directory, with ``/tmp`` as fallback.
Windows uses the operating-system temporary-directory API.

Temporary directories and output streams use cryptographically secure, filename-safe random names and atomic create-new
behavior.
Their handles remove the resource automatically unless cleanup is disabled or ``release()`` transfers the path to the
caller.
Explicit cleanup reports errors and retains the path after a failure so the caller can retry or release it; destructors
never throw.

See :doc:`working_with_paths` for path-value construction, inspection, editing, joining, slicing, and conversion.

Working with Paths
==================

The :cpp:class:`Path <erbsland::path::Path>` class provides a platform-independent representation of filesystem paths.
It separates path manipulation from filesystem access, allowing you to construct, inspect, modify, join, slice, and
convert paths without depending on the operating system or querying the filesystem.

A path stores its logical structure instead of its original textual representation.
It understands POSIX paths, Windows drive paths, and UNC paths, and normalizes them into a consistent internal format.
This makes common path operations predictable and portable across all supported platforms.

See :doc:`working_with_paths` for a complete introduction, including path construction, inspection, editing, joining,
slicing, and conversion between different external path formats.
