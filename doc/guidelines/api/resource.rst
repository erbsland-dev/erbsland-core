********************************
Compiled Resource API Guidelines
********************************

Core Semantics
==============

.. code-block:: text

    resource key = exact case-sensitive pair of portable identifier and normalized UTF-8 relative path
    stored data = exact immutable bytes embedded in the executable
    logical data = original resource bytes after transparent decoding
    missing resource = absent key, distinct from a present resource with zero logical bytes

Primary Types
=============

.. code-block:: text

    Resources // read-only compiled-resource lookup interface
    ResourceManager // lazy process-wide compiled-resource index and decoded-value cache
    ResourceInfo // immutable storage, compression, hashing, and encryption metadata

Secondary Types
===============

.. code-block:: text

    ResourceError, ResourceErrorCategory // required lookup or invalid stored-data failure

Lookup Patterns
===============

.. code-block:: text

    o.contains(identifier, path) -> bool // test an exact resource key
    o.getStoredData(identifier, path) -> optional<ConstByteSpan> // borrow the exact embedded representation
    o.getData(identifier, path) -> optional<ByteBlock> // obtain original logical bytes
    o.getText(identifier, path) -> optional<String> // obtain logical bytes as tolerant UTF-8 text
    o.getInfo(identifier, path) -> optional<ResourceInfo> // obtain immutable resource metadata
    o.get❮Value❯OrThrow(identifier, path) -> T // require a present and valid resource value

Metadata Patterns
=================

.. code-block:: text

    o.originalSize()/storedSize() -> unit::ByteLength // inspect logical and embedded sizes
    o.isCompressed()/compressionAlgorithm() -> T // inspect compression state and algorithm
    o.hasHash()/hashAlgorithm()/hash() -> T // inspect optional logical-data integrity metadata
    o.isEncrypted() -> bool // inspect reserved encryption state
