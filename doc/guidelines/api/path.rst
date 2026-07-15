*******************
Path API Guidelines
*******************

These guidelines extend the Common API Guidelines for public APIs in the ``path`` namespace.
This namespace provides methods to work with the filesystem in a platform agnostic way.
It is intentionally built around a large convenience ``Path`` API.
The API itself handles path management, resolution and manipulation, but is also the entry point to get file/directory
attributes, operations, access file contents and walk directory structures.

Core Semantics
==============

*   **path:** a platform agnostic locator to a resource accessible in the filesystem.
*   **current directory:** the working directory of the process.

Paths
-----

*   **empty is invalid:** Empty paths are invalid and used as error indicator on return values.
*   **current is valid:** A single ``.`` path is valid, points to the current directory.
*   **lazily validated:** Paths are only validated on request or when needed.
    Working with "fictive/abstract" paths or platform foreign paths is ok until the point where:
    a concrete file, directory or resource needs to be accessed. The API is failing at the latest point possible.
*   **kept normalized:** Path objects do minimal normalization when parsing paths from text.
    If a user needs an unchanged path, it has to store it in a string instead.

Normalized Form
---------------

*   Path elements are separated by slash (``/``) characters.
*   A path never ends in a slash.
*   Empty path elements are removed.
*   In absolute paths, the first path element is the whole root.
    This can be a slash ``/`` on POSIX, a drive ``c:/`` or a share ``//server/Share/``.
*   A path root element *always* ends with a slash.

Options
-------

*   Most complex call take ``...Options`` types as input, to keep the API extensible.

Primary Type
============

.. code-block:: text

    Path // a path in the file system and action starting point for related functionality.

Secondary Types
===============

.. code-block:: text

    PathInfo // information about a concrete filesystem object.
    PathWalker // walking directory structures
    PathOperations // basic file/directory operations as copy/move/remove/create empty file/create directory
    PathContent // accessing file contents. Reading/writing text/data or open streams for read/write
    TempDirectory // shared cleanup lease for a temporary directory

Option Types
============

.. code-block:: text

    Path(Copy/Move/Remove)Options // options for copy, move and remove
    PathChangeOptions // options for changing path metadata
    PathCreate(File/Directory)Options // options for creating empty files or directories
    PathTemp(File/Directory)Options // options for creating temporary files or directories
    Path(Read/Write)(Text/Data)Options // options for opening streams
    PathResolveOptions // options when resolving paths
    PathWalkOptions // options when walking paths

Supporting Types
================

.. code-block:: text

    PathCollisionMode, PathCreateMode, PathResolveMode, SymlinkMode // various modes used in options
    PathAccessInfo, PathAccessRight(s), PathAccessProfile // portable access metadata and policies
    PathAttribute(s) // common native file attributes
    PathFormat // detected format of an absolute path
    PathProgressFn, PathProgress, PathProgressStatus // for operation progress callbacks
    PathType // content type of a path. regular file, directory, symlink, etc.
    PathWalkFn, PathWalkDirection, PathWalkResult, PathWalkStatus // for walking callback and result
    PathWindowsFormat // export format for native windows paths.

Path Patterns
=============

.. code-block:: text

    T(path) // create a path from a string or std path
    o / o -> T // join two paths
    o.isEmpty() // test for empty
    o.isValid() // test for os valid
    o.isRelative/isAbsolute/isRoot() // test if root or only root
    o.compare() // compare two paths
    o.format() // get the detected path format (windows/posix)
    o.elementCount() -> I // get the number of elements
    o.element(index) -> V // access one element
    o.elements() -> L // get a list with all elements as strings
    o.parent() -> T // get the parent path
    o.parents() -> L // get a list with all parent paths down to the root path
    o.root/name/suffix/suffixes/stem() -> V // get special parts of a path or filename
    o.withName/withSuffix/withStem() -> T // return a path with one element replaced
    o.join/joined(other) -> T&/T // join paths
    o.slice(range) -> T // get a slice of a path
    o.resolve[OrThrow](options) -> T // resolve a path. canonicalize/normalize.
    o.toAbsolute/toRelative[OrThrow](base) -> T // convert to absolute or relative
    o.commonAncestor(base) // get a common ancestor
    o.info() -> PathInfo // access path info
    o.walker() -> PathWalker // access path walker
    o.content() -> PathContent // access path content
    o.operations() -> PathOperations // access path operations
    o.toStdPath/toPosix/toWindows() // convert to native formats
    o.toString() // convert for display
    T::fromElements(elements) -> T // assemble from strings
    T::fromPosix/fromWindows/fromNative[OrThrow](path) -> T // convert from native
    T::currentDirectory() -> T // get working dir
    T::systemTempDirectory[OrThrow]() -> T // get system temporary dir

PathContent Patterns
====================

.. code-block:: text

    T() // invalid - to allow use as undefined variable
    T(path) // from path
    o.isEmpty() // test for an empty path
    o.path() // access the original path
    o.readText[OrThrow]([options]) -> V // read text from a file
    o.readData[OrThrow]([options [, maximumLength]]) -> B // read byte data from a file
    o.writeText[OrThrow](text [, options]) // write text into a file
    o.writeData[OrThrow](data [, options]) // write byte data into a file
    o.openTextInputStream/openTextOutputStream([options]) -> S // open a text stream for read/write
    o.openByteInputStream/openByteOutputStream([options]) -> S // open a data stream for read/write

PathInfo Patterns
=================

.. code-block:: text

    T() // invalid - to allow use as undefined variable
    T(path) // from path
    o.isEmpty() // test for an empty path
    o.path() // access the original path
    o.exists() // if the path exists
    o.isDirectory/isRegularFile/isSymlink/isDevice/isSocket/isPipe/isReparsePoint() // type test
    o.resolvedPath() // access the resolved path.
    o.type() // get the underlying path type
    o.fileSize() // get the size of a file.
    o.lastModified/lastAccessed/birthTime/lastMetadataChange/creationTime() // date/time attributes
    o.ownerName/ownerId/groupName/groupId[OrThrow]() // owner and group attributes
    o.accessInfo[OrThrow]() // get portable access information
    o.isReadable/isWritable/isExecutable() // current-process access tests
    o.attributes[OrThrow]() // get common native file attributes
    o.hasAttribute(attribute) // test a native attribute
    o.reload([parts]); // invalidate cache and refresh from filesystem

PathOperations Patterns
=======================

.. code-block:: text

    T() // invalid - to allow use as undefined variable
    T(path) // from path
    o.isEmpty() // test for an empty path
    o.path() // access the original path
    o.remove[OrThrow]([options [, progressFn]]) // remove a file, directory, tree
    o.copyTo[OrThrow](destination [, options [, progressFn]]) // copy a file, directory, tree
    o.moveTo[OrThrow](destination [, options]) // move, rename a path
    o.createFile[OrThrow](options) // create empty file (touch)
    o.createDirectory[OrThrow](options) // create directory
    o.createTempDirectory[OrThrow](options) -> S // create a temporary directory under this path
    o.openTempByteOutputStream[OrThrow](options) -> S // create and open a temporary byte output file
    o.openTempTextOutputStream[OrThrow](temporaryOptions, writeOptions) -> S // temporary text output file
    o.setAccessProfile[OrThrow](profile, options) // change portable access profile
    o.addAttributes[OrThrow](attributes, options) // add native attributes
    o.clearAttributes[OrThrow](attributes, options) // clear native attributes

TempDirectory Patterns
======================

.. code-block:: text

    T() // empty temporary directory handle
    o.isEmpty() // test for an empty handle
    o.path() // access the temporary directory path
    o.removeOnDestroy() // test if the directory is removed when the handle is destroyed
    o.setRemoveOnDestroy(value) // set if the directory is removed when the handle is destroyed
    o.release() -> Path // disable automatic cleanup and return the path
    o.remove[OrThrow]() // remove the temporary directory now

Temporary File Stream Patterns
==============================

.. code-block:: text

    o.isEmpty() // test for an empty temporary file stream
    o.path() // access the temporary file path
    o.removeOnClose() // test if the file is removed when the stream is closed or destroyed
    o.setRemoveOnClose(value) // set if the file is removed when the stream is closed or destroyed
    o.release() -> Path // disable automatic cleanup and return the path

PathWalker Patterns
===================

.. code-block:: text

    T() // invalid - to allow use as undefined variable
    T(path) // from path
    o.isEmpty() // test for an empty path
    o.path() // access the original path
    o.walk[OrThrow](walkFn [, options]) // walk a dir tree
