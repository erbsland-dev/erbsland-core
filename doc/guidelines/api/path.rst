*******************
Path API Guidelines
*******************

Core Semantics
==============

Path Model
----------

.. code-block:: text

    empty path = invalid value and non-throwing failure sentinel
    dot path = valid current-directory value
    validation = lazy until explicit validation or native filesystem access
    representation = minimally normalized portable text
    separator = forward slash
    root = complete first element such as slash, drive root, or UNC share
    trailing separator = absent except when it is part of the root

Filesystem Model
----------------

.. code-block:: text

    information = snapshot cached on Path and shared by its value copies, refreshed explicitly or after expiry
    temporary resource = owned cleanup lease releasable to the caller

Primary Types
=============

.. code-block:: text

    Path // platform-independent path value and filesystem-operation entry point

Operation Types
===============

.. code-block:: text

    PathInfo // cached filesystem metadata accessor
    PathContent // complete-data and stream accessor
    PathOperations // filesystem mutation accessor
    PathWalker // directory-tree traversal accessor
    TempDirectory // shared cleanup lease for a temporary directory
    PathError, PathErrorContext // filesystem failure and structured diagnostic context

Option Types
============

.. code-block:: text

    PathCopyOptions, PathMoveOptions, PathRemoveOptions // tree mutation policies
    PathCreateFileOptions, PathCreateDirectoryOptions // file and directory creation policies
    PathTempFileOptions, PathTempDirectoryOptions // temporary resource policies
    PathReadDataOptions, PathWriteDataOptions // binary content policies
    PathReadTextOptions, PathWriteTextOptions // text content and encoding policies
    PathResolveOptions, PathWalkOptions, PathChangeOptions // resolution, traversal, and metadata policies

Value Types
===========

.. code-block:: text

    PathType, PathTypes, PathFormat // filesystem object and path-format classifications
    PathInfoPart, PathInfoParts // metadata fields loaded into a snapshot
    PathAccessInfo, PathAccessRight, PathAccessRights // portable access metadata
    PathAccessProfile // predefined portable access policy
    PathAttribute, PathAttributes // common native file attributes
    PathCollisionMode, PathCreateMode, PathResolveMode, SymlinkMode // operation policies
    PathProgress, PathProgressStatus, PathProgressFn // operation progress data and callback
    PathWalkDirection, PathWalkStatus, PathWalkResult, PathWalkFn // traversal control and result data
    PathWindowsFormat // native Windows export format

Pattern Definitions
===================

.. code-block:: text

    A = PathInfo/PathContent/PathOperations/PathWalker // path-bound operation accessor
    O = Path❮Operation❯Options // options matching one filesystem operation

Path Value Patterns
===================

.. code-block:: text

    T(path) // create from portable text or a standard-library path
    o.isEmpty()/isValid() -> bool // test the sentinel state or current-platform validity
    o.isRelative/isAbsolute/isRoot() -> bool // classify path form
    o.elementCount()/element(index)/elements() -> T // inspect normalized path elements
    o.root()/parent()/parents() -> T // inspect the path hierarchy
    o.name()/stem()/suffix()/suffixes() -> T // inspect file-name parts
    o.withName/withStem/withSuffix(value) -> Path // replace one file-name part
    o.join/joined(path) -> Path // append in place or return a joined value
    o.slice(range)/commonAncestor(other) -> Path // derive structural path values
    o.resolve/resolveOrThrow([options]) -> Path // canonicalize with non-throwing or throwing failure
    o.toAbsolute/toRelative([base]) -> Path // convert path form with an empty failure sentinel
    o.toAbsoluteOrThrow/toRelativeOrThrow([base]) -> Path // convert path form or throw PathError
    o.toString()/toStdPath()/toPosix()/toWindows() -> T // export portable or native representations
    T::fromElements/from❮Format❯(value) -> Path // import normalized elements or a native representation
    T::currentDirectory/userHomeDirectory/systemTempDirectory() -> Path // query a platform directory

Operation Accessor Patterns
===========================

.. code-block:: text

    o.info()/content()/operations()/walker() -> A // create a path-bound operation accessor
    T(path) // create an accessor for a path
    o.isEmpty() -> bool // test for an invalid bound path
    o.path() -> Path // inspect the bound path
    o.❮operation❯([options]) -> T // perform an operation with non-throwing failure reporting
    o.❮operation❯OrThrow([options]) -> T // perform the same operation or throw PathError

Read and Write Patterns
=======================

.. code-block:: text

    o.readText/readData([options]) -> T // read complete file content or return a failure sentinel
    o.readTextOrThrow/readDataOrThrow([options]) -> T // read complete content or throw PathError
    o.writeText/writeData(value[, options]) -> bool // write complete content with status reporting
    o.writeTextOrThrow/writeDataOrThrow(value[, options]) // write complete content or throw PathError
    o.open❮Kind❯InputStream([options]) -> T // open a byte or text input stream
    o.setSymlinkMode(mode) -> O& // configure whether an input operation may follow symbolic links
    o.open❮Kind❯OutputStream([options]) -> T // open a byte or text output stream

Mutation and Temporary Resource Patterns
========================================

.. code-block:: text

    o.remove/copyTo/moveTo([destination, options, progress]) -> bool // mutate with status reporting
    o.removeOrThrow/copyToOrThrow/moveToOrThrow([destination, options, progress]) // mutate or throw PathError
    o.createFile/createDirectory([options]) -> bool // create a filesystem object with status reporting
    o.createTempDirectory([options]) -> TempDirectory // create a directory cleanup lease
    o.openTemp❮Kind❯OutputStream([options]) -> T // create a temporary output stream cleanup lease
    o.setAccessProfile/addAttributes/clearAttributes(value[, options]) -> bool // change portable metadata
    o.path()/release() -> Path // inspect or release a temporary resource path
    o.removeOnDestroy()/setRemoveOnDestroy(enabled) // configure directory cleanup
    o.removeOnClose()/setRemoveOnClose(enabled) // configure temporary stream cleanup

Information Patterns
====================

.. code-block:: text

    o.reload([parts]) // refresh selected cached metadata
    o.exists()/is❮PathType❯() -> bool // test existence or object type
    o.type()/resolvedPath()/fileSize() -> T // inspect core filesystem metadata
    o.❮timestamp❯() -> time::DateTime // inspect a loaded filesystem timestamp
    o.ownerName/ownerId/groupName/groupId() -> T // inspect ownership with an empty failure sentinel
    o.accessInfo()/attributes() -> T // inspect portable access or native attributes
    o.isReadable/isWritable/isExecutable() -> bool // test current-process access

Traversal Patterns
==================

.. code-block:: text

    o.walk(callback[, options]) -> PathWalkResult // traverse with explicit result reporting
    o.walkOrThrow(callback[, options]) -> PathWalkResult // traverse or throw PathError
    o.set❮Property❯(value) -> O& // fluently configure an operation option object
