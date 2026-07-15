****************************
Memory Domain API Guidelines
****************************

These guidelines extend the Common, Text, and Unit and Value API Guidelines for public APIs that model bytes, byte
blocks, shared storage, and unsafe memory boundaries.

The purpose of this document is to define a base naming vocabulary for memory APIs.
It is intentionally plain, technical and list based to get a quick overview of method names and their usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

Storage Vocabulary
------------------

.. code-block:: text

    Byte // explicit single-byte value
    ByteBlock // owning copy-on-write byte sequence
    ByteBlockView // read-only shared byte sequence view
    CowStorage // automatic-detach copy-on-write wrapper
    CowManualStorage // explicit-detach copy-on-write wrapper
    SharedDataPointer // intrusive copy-on-write pointer for library storage internals
    StorageIdentifier // identity token for a backend storage range
    Unsafe❮Type❯Ptr // deliberate raw pointer boundary

Byte Ranges
-----------

.. code-block:: text

    unit::ByteIndex // index into byte data
    unit::ByteLength // byte count or capacity
    unit::ByteRange // half-open byte range
    endIndex() // first index after the visible byte sequence
    get(index, defaultValue) // tolerant byte access
    getOrThrow(index) // strict byte access

Primary Types
=============

.. code-block:: text

    Byte // wrapper around one uint8_t value
    ByteBlock // copy-on-write byte container
    ByteBlockView // read-only shared view into byte block data
    ByteReader // sequential byte and integer reader
    ByteWriter // sequential byte and integer writer
    Endianness // byte order for multi-byte integer IO
    StorageIdentifier // stable identity for storage ranges

Storage and Pointer Types
=========================

.. code-block:: text

    CowStorage // shared copy-on-write storage with automatic mutable detach
    CowManualStorage // shared copy-on-write storage with explicit mutable detach
    SharedData // base for intrusive shared data objects
    SharedVirtualData // polymorphic shared data base with virtual clone
    SharedArrayData // compact shared trailing-array storage
    SharedDataPointer // intrusive shared-data pointer with optional manual detach
    ReferenceCounter // atomic intrusive reference counter
    UnsafeConstMemoryPtr, UnsafeMemoryPtr // raw memory pointer aliases
    UnsafeConstCharPtr, UnsafeCharPtr // raw char pointer aliases
    UnsafeConstChar8Ptr, UnsafeChar8Ptr // raw char8_t pointer aliases
    impl::UnsafeByteBlockBuffer // uncommitted ByteBlock storage for native byte reads

Byte Value Patterns
===================

.. code-block:: text

    Byte(value) // create from uint8_t
    o.toRawValue() -> uint8_t // return raw byte value
    o.toUInt8() -> uint8_t // return byte as unsigned integer
    o.masked(mask) -> uint8_t // apply bit mask
    o.matches(mask, expected) -> bool // test masked bits against expected value

Byte Sequence Patterns
======================

.. code-block:: text

    ByteBlock(length[, value]) // create repeated byte value
    ByteBlock(span/vector) // create byte block from raw byte values
    ByteBlockView(block) // create view to a full byte block
    o.isEmpty() -> bool // test for no visible bytes
    o.startsWith/endsWith(bytes) -> bool // prefix or suffix test
    o.contains(bytes) -> bool // byte sequence membership
    o.length() -> unit::ByteLength // visible byte length
    o.endIndex() -> unit::ByteIndex // first index after visible bytes
    o.get(index[, defaultValue]) -> Byte // tolerant indexed byte access
    o.getOrThrow(index) -> Byte // strict indexed byte access, throws err::OutOfRangeError
    o.set(index, byte) -> void // tolerant indexed byte write
    o.setOrThrow(index, byte) -> void // strict indexed byte write, throws err::OutOfRangeError
    o.slice(range/begin, end-or-length) -> ByteBlockView // shared byte slice
    o.find(bytes[, start]) -> unit::ByteIndex // first byte-sequence occurrence
    o.findLast(bytes) -> unit::ByteIndex // last byte-sequence occurrence
    o.toByteVector() -> vector<Byte> // materialize explicit byte values
    o.toUInt8Vector() -> vector<uint8_t> // materialize raw unsigned byte values
    o.toCharVector() -> vector<char> // materialize raw char values

Byte Block Mutation Patterns
============================

.. code-block:: text

    o.clear() -> ByteBlock& // remove bytes while keeping capacity
    o.reset() -> void // remove bytes and release storage
    o.remove(range) -> ByteBlock& // remove byte range in-place
    o.keep(range) -> ByteBlock& // keep only byte range in-place
    o.replace(range, bytes) -> ByteBlock& // replace range in-place
    o.insert(index, bytes) -> ByteBlock& // insert bytes, appending for out-of-range index
    o.append(byte/bytes) -> ByteBlock& // append bytes
    o.removed(range) -> ByteBlock // return copy with range removed
    o.replaced(range, bytes) -> ByteBlock // return copy with range replaced
    o.join(parts) -> ByteBlock // join byte blocks using this block as separator
    T::fromJoined(parts) -> ByteBlock // join byte blocks without a separator

Capacity and Identity Patterns
==============================

.. code-block:: text

    o.detach() -> void // ensure unique byte or data storage
    o.capacity() -> unit::ByteLength // current storage capacity
    o.reserve(capacity) -> void/T& // reserve storage capacity
    o.shrinkToFit() -> void // release unused storage
    o.isShared() -> bool // test whether storage is shared
    o.useCount() -> integer // current shared reference count
    o.storageId() -> std::size_t // raw implementation identity for diagnostics
    StorageIdentifier::fromMemoryRange(begin, end) -> StorageIdentifier // identity for visible storage range
    o.toRawValues() -> array<uint64_t, 2> // mixed storage identity values

Reader and Writer Patterns
==========================

.. code-block:: text

    ByteReader(block/view) // create sequential reader
    ByteWriter() // create sequential writer
    o.length() -> unit::ByteLength // readable or written byte length
    o.position() -> unit::ByteIndex // current read or write position
    o.setPosition(index) -> void // move position, clamped to length
    o.endianness() -> Endianness // byte order for integer helpers
    o.setEndianness(value) -> void // set byte order for integer helpers
    o.isAtEnd() -> bool // test if reader reached the end
    o.canRead(byteCount) -> bool // test if reader has enough remaining bytes
    o.advance(byteCount) -> void // move reader forward, clamped to length
    o.toByteBlock() -> ByteBlock // materialize written bytes
    o.readByte([default]) -> Byte // read one byte and advance, or default at end
    o.peekByte([offset][, default]) -> Byte // read without advancing, or default at end
    o.readByteOrThrow()/peekByteOrThrow() -> Byte // strict byte read, throws err::OutOfRangeError
    o.readInteger<T>(default) -> T // tolerant integer read with endianness
    o.readIntegerOrThrow<T>() -> T // strict integer read, throws err::OutOfRangeError
    o.readIntegerInto(value) -> bool // read into existing value, false if not enough bytes
    o.writeByte(byte) -> ByteWriter& // write one byte and advance
    o.writeInteger(value) -> ByteWriter& // write integer with configured endianness
    o.read❮SignWidth❯([default]) -> T // typed tolerant integer reader, e.g. readUInt32()
    o.read❮SignWidth❯OrThrow() -> T // typed strict integer reader
    o.write❮SignWidth❯(value) -> ByteWriter& // typed integer writer

Unsafe Byte Buffer Patterns
===========================

.. code-block:: text

    UnsafeByteBlockBuffer(capacity) // allocate uncommitted byte block storage
    o.data() -> span<Byte> // writable byte span for low-level APIs
    o.capacity() -> ByteLength // usable byte capacity
    o.take(length) -> ByteBlock // commit length bytes and move out the byte block

Copy-On-Write Storage Patterns
==============================

.. code-block:: text

    T::from(data) -> T // create unique storage from existing data
    T::create(args...) -> T // create unique storage by constructing data in place
    o.data() const -> const Data& // read stored data
    o.data() -> Data& // mutable data access with automatic detach
    o.detachedData() -> Data& // mutable data access with explicit detach naming
    o.setData(data) -> void // replace stored data with a unique object
    o.emplaceData(args...) -> void // replace by constructing a unique object in place
    o.detach() -> void // ensure unique ownership
    o.swap(other) -> void // exchange storage
    swap(a, b) -> void // exchange storage objects

Advanced Shared Data Patterns
=============================

.. code-block:: text

    SharedDataPointer(data) // take intrusive shared ownership of newly allocated data
    o.get()/constGet() -> Data* // mutable or const pointer access
    o.reset([data]) -> void // replace managed data
    o.isNull() -> bool // test for no managed data
    o.operator*/operator-> // access managed data, detaching unless manual-detach mode is enabled
    SharedArrayData::create(size, capacity) -> Data* // allocate unreferenced trailing-array storage
    o.clone() -> Data* // create unreferenced detached copy
    SharedArrayData::destroy(data) -> void // destroy trailing-array storage
    T::canAllocateWithCapacity(capacity) -> bool // test allocation preconditions
    T::allocationOverhead() -> std::size_t // bytes before trailing element storage
    T::allocationSizeForCapacity(capacity) -> std::size_t // total allocation size
    o.addReference()/removeReference() -> ReferenceStatus // intrusive reference count changes
    o.isReferenced()/status() -> bool/ReferenceStatus // reference counter state
