****************************
Memory Domain API Guidelines
****************************

Core Semantics
==============

.. code-block:: text

    borrowed view = non-owning contiguous bytes valid until source mutation or destruction
    direct owner = independent allocation copied deeply and transferred by move
    shared block = immutable value or slice sharing read-only allocation ownership
    block editor = mutable copy-on-write value that detaches before modification
    marked allocation = one-way marker that is passed to all copies of an object.
    secure erasure = optimizer-resistant overwrite of complete owned capacity
    constant-time equality = explicit comparison without content-dependent short-circuiting for equal lengths

Primary Types
=============

.. code-block:: text

    Byte // explicit single-byte value
    ByteSpan, ConstByteSpan // mutable and read-only dynamic byte views
    FixedByteSpan❮extent❯, FixedConstByteSpan❮extent❯ // fixed-extent byte views
    ByteArray❮size❯ // fixed-size mutable byte owner
    ByteBuffer // dynamic deep-copying mutable byte owner
    ByteBlock // shared read-only byte value and slice

Secondary Types
===============

.. code-block:: text

    Endianness // little- or big-endian integer byte order
    ByteIntegerFormat // explicit signed or unsigned static/variable integer wire format
    ByteTextFormat, ByteTextOptions // dynamic or padded text framing definition
    ByteBlockEditor // explicit mutable copy-on-write byte value
    RingBuffer, ByteRingBuffer // bounded FIFO bytes with optional integer operations
    BitReader // sequential single-bit reader over borrowed read-only bytes
    ByteReader, ByteWriter // sequential byte and integer reader and writer
    StorageIdentifier // identity token for a visible backend storage range
    CowStorage❮Data❯, CowManualStorage❮Data❯ // automatic- and explicit-detach copy-on-write wrappers
    SharedData, SharedVirtualData // intrusive shared-data bases
    SharedArrayData❮Value❯ // intrusive trailing-array storage
    SharedDataPointer❮Data❯ // intrusive shared-data pointer with optional manual detach
    ReferenceCounter // atomic intrusive reference state
    UnsafeConstMemoryPtr, UnsafeMemoryPtr // explicit raw memory boundaries
    UnsafeConstCharPtr, UnsafeCharPtr // explicit raw character boundaries

Compression Types
=================

.. code-block:: text

    ByteCompressionAlgorithm // stable raw compression algorithm identifier and output bound
    ByteCompressor, ByteDecompressor // one-shot and buffered byte compression
    ByteCompressionError, ByteCompressionErrorReason // malformed or unsupported representation failure

Pattern Definitions
===================

.. code-block:: text

    B = ByteArray❮size❯/ByteBuffer/ByteBlock/ByteBlockEditor // owning byte sequence
    S = ByteSpan/ConstByteSpan/FixedByteSpan❮size❯/FixedConstByteSpan❮size❯ // borrowed byte sequence

Internal Byte Tool Patterns
===========================

.. code-block:: text

    o.dataView() -> impl::ByteDataView // private raw view shared by contiguous byte owners
    byteReadTools(view) // bind ByteReadTools for read, iteration, conversion, slicing, and integer algorithms
    byteComparisonTools(view) // bind ByteComparisonTools for comparison and search algorithms
    o.writableSpan() -> ByteSpan // private fixed-size mutation boundary for writable owners
    byteWriteTools(span) // bind ByteWriteTools for raw set, fill, overwrite, XOR, and integer writes
    byteModifyTools(owner) // bind ByteModifyTools for COW, aliasing, sensitivity, allocation, and resizing
    operationalTools(dataOrService) // bind operational tools to the data or service they operate on
    staticHelper(...) // allow static helpers only as subordinate details of a stateful tool
    o.validateWrite(...) // reject ineffective writes before requesting a span that can detach or reallocate
    algorithm(view) // do not repeatedly traverse shared storage or create another owning byte object

Byte Value Patterns
===================

.. code-block:: text

    T(value) // create from a raw byte-compatible value
    o.toStdByte()/toUInt8()/toChar() -> T // cross an explicit raw-value boundary
    o.shifted❮Direction❯/rotated❮Direction❯(amount) -> Byte // return transformed bits
    o.shift❮Direction❯/rotate❮Direction❯(amount) // transform bits in place
    o.masked(mask) -> Byte // return masked bits
    o.matches(mask, expected) -> bool // test masked bits against an expected value

Byte Sequence Read Patterns
===========================

.. code-block:: text

    o.isEmpty() -> bool // test for no visible bytes
    o.length()/endIndex() -> T // inspect visible sequence bounds
    o.get(index[, fallback]) -> Byte // tolerant indexed access
    o.getOrThrow(index) -> Byte // strict indexed access
    o.span([range]) -> ConstByteSpan // borrow complete or clamped visible bytes
    o.isEqualConstTime(bytes) -> bool // compare every byte when lengths match; lengths remain observable
    o.startsWith/endsWith/contains(bytes) -> bool // test byte-sequence membership
    o.find/findLast(bytes[, start]) -> unit::ByteIndex // locate a byte sequence
    o.forEach(function) -> util::LoopResult // visit bytes with an optional index
    o.toByteBuffer() -> ByteBuffer // create an independent mutable byte copy

Mutable Byte Sequence Patterns
==============================

.. code-block:: text

    T(length[, value])/T(bytes) // create owned byte storage
    T::fromSpan(span) -> T // explicitly copy borrowed bytes for dynamic owners
    T::fromSpan(span) -> std::optional❮T❯ // copy exact-size bytes into a fixed array, or reject a mismatch
    T::fromSpanOrThrow(span) -> T // strictly copy an exactly sized borrowed sequence into a fixed array
    o.set/setOrThrow(index, byte) // tolerant or strict indexed write
    o.fill([range], byte) // fill complete or clamped visible storage
    o.overwrite([range], bytes) // copy the largest fitting source prefix without resizing
    o.xorAt/xorAtOrThrow(index, byte) // tolerant or strict indexed XOR
    o.xorWith([range], bytes) -> T // apply bulk XOR with type-specific size behavior
    o.xorWithOrThrow(bytes) -> T // require equal lengths for whole-sequence XOR
    o.append/insert/replace(position, bytes) -> T& // resize and add or replace bytes
    o.remove/keep(range) -> T& // remove or retain a clamped range
    o.resize/reserve/shrinkToFit(capacity) // manage visible length and capacity
    o.clear()/reset() // empty while retaining or releasing storage

Shared Block Patterns
=====================

.. code-block:: text

    T(editor-or-block) // share visible block allocation ownership
    o.slice(range) -> ByteBlock // create a shared read-only slice
    o.join(parts) -> ByteBlockEditor // join byte sequences using the receiver as separator
    T::fromJoined(parts) -> ByteBlockEditor // join byte sequences without a separator
    o.detach() // ensure unique mutable storage before low-level access

Integer Access Patterns
=======================

.. code-block:: text

    getInteger❮T❯(bytes, offset[, endianness, fallback]) -> T // tolerant endian-aware read
    getIntegerInto(bytes, value, offset[, endianness]) -> bool // checked endian-aware read
    getIntegerOrThrow❮T❯(bytes, offset[, endianness]) -> T // strict endian-aware read
    setInteger(bytes, offset, value[, endianness]) -> bool // checked endian-aware write
    setIntegerOrThrow(bytes, offset, value[, endianness]) // strict endian-aware write
    toByteSpan/toConstByteSpan(stdSpan) -> S // create a zero-copy compatibility view

Sequential Read and Write Patterns
==================================

.. code-block:: text

    T(block-or-editor) // create a reader sharing its input data
    T([endianness]) // create an empty sequential writer
    o.position()/length() -> T // inspect sequential bounds
    o.isAtEnd() -> bool // test whether a sequential reader reached its end
    o.setPosition/advance(amount) // move within clamped bounds
    o.readByte/peekByte([fallback]) -> Byte // tolerant advancing or non-advancing read
    o.readByteOrThrow/peekByteOrThrow() -> Byte // strict advancing or non-advancing read
    o.readInteger❮T❯([fallback])/readIntegerOrThrow❮T❯() -> T // compatibility native-width integer read
    o.readInteger❮T❯(format) -> optional❮T❯ // transactional formatted integer read
    o.readIntegerOrThrow❮T❯(format) -> T // strict formatted integer read
    o.readBytes(length) -> optional❮ByteBlock❯ // transactional exact byte read
    o.readBytesOrThrow(length) -> ByteBlock // strict exact byte read
    o.readText(options) -> optional❮String❯ // transactional structured text read
    o.readTextOrThrow(options) -> String // strict structured text read
    o.writeByte/writeInteger(value)/writeIntegerOrThrow(value, format) -> ByteWriter& // append and advance
    o.writeText(text, options)/writeTextOrThrow(text, options) -> ByteWriter& // truncating or strict text frame
    o.toByteBlock() -> ByteBlock // materialize written bytes

Ring Buffer Patterns
====================

.. code-block:: text

    T(capacity[, maximumCapacity]) // create a fixed or growable bounded ring
    o.length()/available()/capacity()/maximumCapacity() -> unit::ByteLength // inspect ring bounds
    o.write(bytes)/read(destination) -> unit::ByteLength // transfer as many bytes as possible
    o.writeExact(bytes) -> util::Result // atomically write all bytes or leave the ring unchanged
    o.read(maximum) -> ByteBlock // remove up to the requested byte count
    o.readInteger() -> T // atomically read one endian-aware integer
    o.writeInteger(value) -> util::Result // atomically write one endian-aware integer
    o.clear()/shrinkToInitial() // discard bytes or restore empty initial capacity

Byte Compression Patterns
=========================

.. code-block:: text

    T(algorithm) // create a raw compressor or decompressor for one algorithm
    o.compress(bytes)/decompress(bytes, originalSize) -> ByteBlock // transform a raw algorithm block
    o.compressWithEnvelope(bytes) -> ByteBlock // create a framed self-describing representation
    T::decompressWithEnvelope(bytes[, maximumSize]) -> ByteBlock // validate, dispatch, and decode an envelope
    o.update(bytes) // append buffered incremental input before finalization
    o.finalize([originalSize])/finalizeWithEnvelope([maximumSize]) -> ByteBlock // finalize in one selected format
    o.reset() // discard buffered input and cached output for reuse

Sensitive Byte Storage Patterns
===============================

.. code-block:: text

    o.isSensitive() -> bool // inspect the shared-allocation mark or direct-owner mode
    o.markAsSensitive() // irreversibly mark ByteBlock or ByteBlockEditor shared storage
    o.setSensitive(enabled) // configure reversible ByteBuffer or RingBuffer sensitive mode
    o.secureErase() // erase complete capacity while preserving visible length and sensitivity
    o.copy/slice/mutate(...) -> T // preserve or propagate sensitivity according to owning-type semantics

Shared Storage Patterns
=======================

.. code-block:: text

    T::create/from(data) -> T // create unique shared storage
    o.data()/detachedData() -> T& // access data with automatic or explicit detach semantics
    o.setData/emplaceData(value) // replace storage with a unique object
    o.detach() // ensure unique shared storage
    o.isShared()/useCount() -> T // inspect shared ownership
    o.reset([data])/swap(other) // replace or exchange intrusive storage
