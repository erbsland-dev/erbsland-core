*********************************
Compression Domain API Guidelines
*********************************

Primary Types
=============

.. code-block:: text

    CompressionAlgorithm // stable Core-envelope algorithm identifier
    CompressionFormat // Raw, Zip, or Core representation selected at construction
    CompressionLevel // portable human-facing compression effort
    ByteCompressor, ByteDecompressor // one-shot and stream-to-stream byte compression

Secondary Types
===============

.. code-block:: text

    CompressionOptions // finite encoder workspace policy
    CompressionTransferOptions, CompressionTransferResult // one transfer policy and cumulative byte counts
    CompressionProgress, CompressionPhase, CompressionProgressAction // synchronous progress and cancellation
    CompressionFallbackPolicy // capped whole-value fallback or required native streaming
    DecompressionOptions // exact-output validation and finite output/workspace policies
    CompressionError, CompressionErrorContext, CompressionErrorReason // structured stream failure and context

Archive Types
=============

.. code-block:: text

    zip::ArchiveReader, zip::ArchiveItem // inspect and extract one open ZIP archive
    zip::ArchiveWriter // append entries and finalize one new ZIP archive
    zip::CompressionMethod // ZIP method number, distinct from CompressionAlgorithm
    zip::ArchiveReaderOptions, zip::ArchiveWriterOptions // archive-wide resource policies
    zip::ArchiveEntryOptions // one-entry compression, time, and comment overrides
    zip::ArchiveExtractionOptions // collision, timestamp, atomic-write, and output policies
    zip::ArchiveDirectoryOptions // traversal, glob, callback, and entry policies
    zip::ZipProgress // item path, operation phase, and byte-transfer progress
    zip::ZipError, zip::ZipErrorContext, zip::ZipErrorReason // structured archive failure and context

Archive Patterns
================

.. code-block:: text

    T::create(path-or-bytes-or-owned-stream[, options]) -> Tp // open a reader or writer
    o.item(index) -> ArchiveItemPtr // bounded nullable directory lookup
    o.items() -> ArchiveItemList // immutable directory snapshot
    o.extract(maximumLength) -> ByteBlock // bounded decode with length and CRC validation
    o.extractToStream(output-stream[, options]) // borrowed output with final integrity validation
    o.extractToDirectory(root[, options]) // safe item or bulk extraction
    o.setCompressionMethod(value) // set the persistent method for following entries
    o.setCompressionLevel(value) // set the persistent level for following entries
    o.addFile(path[, storage-path]) // append one filesystem file
    o.addData(bytes, storage-path) // append one byte value
    o.addStream(stream, storage-path[, options/length]) // own and append a byte stream
    o.addDirectory(path[, options]) // append a filtered directory tree
    o.addItem(item) // copy validated structure and compressed payload without recompression
    o.finalize() // required, idempotent successful completion
    o.abort() // immediate terminal cancellation

Byte Compression Patterns
=========================

.. code-block:: text

    T(algorithm, format[, level/options]) // immutable codec configuration
    o.compress(bytes)/decompress(bytes) -> ByteBlock // transform one complete value
    o.maximumCompressedLength(length) -> ByteLength // configured representation bound
    o.compress(input-stream, output-stream[, transfer-options]) -> CompressionTransferResult // borrowed streams
    o.decompress(input-stream, output-stream[, transfer-options]) -> CompressionTransferResult // borrowed streams
    o.supportsStreaming() // native bounded-memory support for the configured direction and representation
