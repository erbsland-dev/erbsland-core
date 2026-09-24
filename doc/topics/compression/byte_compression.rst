.. index::
    single: Compression; Byte compression
    single: Compression; Raw representation
    single: Compression; ZIP payload
    single: Compression; Core envelope
    single: ByteCompressor
    single: ByteDecompressor

****************
Compressing Data
****************

Compression is most useful when it disappears into the design of an application: bytes become smaller before they are
stored or transferred, and the exact original bytes return when they are needed again.
The interesting decisions happen around that simple journey.
You need an algorithm that suits the data, a representation that fits the surrounding format, and limits that keep
decompression predictable even when the input is not trusted.

This page leads you through those decisions and shows how to compress complete values or values that arrive in pieces.
The examples use the same small, dependency-free API for every supported algorithm.
If you would first like to understand how a particular algorithm transforms its input, follow the links in the table
below to its dedicated topic page.

Byte Compression and Supported Algorithms
=========================================

Lossless byte compression replaces recurring information with a more compact description.
When the description is decoded, every byte returns exactly as it was; there is no approximation and no change in the
meaning of the data.
Repeated text, structured records, and recurring binary fields often offer useful patterns.
Encrypted data, images or audio that are already compressed, and very small values usually offer much less.

A compressed representation also has its own structure.
It may need block headers, tables, checksums, or a description of the original size.
For this reason, a compressor cannot promise that every result is smaller than its input.
A surrounding file format can compare both sizes and store the original bytes when compression would not be worthwhile.
The byte-compression API always returns a valid compressed value and leaves that storage decision to the application.

There is no universally best algorithm.
Fast matching is attractive for short-lived application data, while deeper searches and statistical coding can be a
better investment for archives.
Large dictionaries may find patterns across a wider distance, but they also increase the memory a decoder must reserve.
Compatibility can matter more than either speed or size when another implementation must read the result.

:cpp:class:`CompressionAlgorithm <erbsland::compression::CompressionAlgorithm>` names the five choices available in
Erbsland Core:

.. list-table:: Supported compression algorithms
    :header-rows: 1
    :widths: 18 25 13 44

    * - Algorithm
      - Raw representation
      - ZIP method
      - A good starting point when
    * - :doc:`LZ4 <lz4>`
      - Raw LZ4 block
      - —
      - Very fast decoding and small, independent values matter most.
    * - :doc:`Deflate <deflate>`
      - RFC 1951 stream
      - 8
      - Broad compatibility with established protocols and tools is important.
    * - :doc:`bzip2 <bzip2>`
      - Complete ``BZh`` stream
      - 12
      - Repetitive text or structured data justifies more compression work.
    * - :doc:`LZMA <lzma>`
      - LZMA-Alone stream
      - 14
      - A compact archival result matters more than encoding time and memory.
    * - :doc:`Zstandard <zstandard>`
      - Zstandard frame
      - 93
      - A modern framed representation and quick decoding are useful.

Choosing Raw, ZIP, and Core Representations
===========================================

An algorithm describes how bytes are transformed, but applications also need to know where one compressed value ends and
which information accompanies it.
:cpp:enum:`CompressionFormat <erbsland::compression::CompressionFormat>` makes that surrounding representation an
explicit part of the compressor and decompressor configuration.
Keeping it explicit is safer than guessing from a few leading bytes, especially when compressed data crosses a trust
boundary.

``Raw`` selects the algorithm's ordinary standalone representation.
For Deflate this is an RFC 1951 stream; for bzip2 it is a complete ``BZh`` stream; for LZMA it is LZMA-Alone; and for
Zstandard it is a standard frame.
LZ4 is different because its raw block contains neither the original size nor an end marker.
Its surrounding format must therefore preserve both the compressed boundary and exact output length.

``Zip`` produces the compressed *payload of one ZIP entry*.
It does not write the local file header, file name, CRC fields, central directory, or any other part of a ZIP archive.
Deflate, bzip2, and Zstandard use the same bytes in ``Raw`` and ``Zip`` because ZIP stores their method number outside
the payload.
LZMA uses the small method-14 header specified by ZIP instead of the LZMA-Alone header.
ZIP has no LZ4 payload representation supported by this API, so that constructor combination is rejected.

``Core`` is convenient when an application-owned value must stand on its own.
It wraps the algorithm's ``Raw`` representation in the version-2 Erbsland Core compression envelope:

.. code-block:: text

    Header       : ELBC | version 2 | algorithm | zero flags (16 bits)
    Payload      : repeated [chunk length (32 bits) | 1..65536 codec bytes]
    Terminator   : zero chunk length (32 bits)
    Trailer      : original length (64 bits) | payload length (64 bits) | CRC-32

All integers are little-endian.
The algorithm identifier is independent of ZIP method numbers.
Chunks carry one continuous raw codec representation.
Writers fill each chunk except the final one.
Version 1 is rejected.
The trailer permits streaming without knowing either final length in advance.

A Core decompressor still receives an expected algorithm from trusted application context.
It verifies that expectation against the embedded identifier instead of silently dispatching to arbitrary data.
It also requires the payload to end at the declared boundary, so an otherwise valid value followed by trailing bytes is
not accepted.

Compressing Complete Values
===========================

Construct :cpp:class:`ByteCompressor <erbsland::compression::ByteCompressor>` with an algorithm, representation, and
optional :cpp:enum:`CompressionLevel <erbsland::compression::CompressionLevel>`.
The default level balances compression time and result size for the chosen codec.
The five portable levels describe human intent; they are translated into suitable search depths, block sizes, or
dictionary sizes rather than written into a Core-specific metadata field.

:cpp:func:`ByteCompressor::compress() <erbsland::compression::ByteCompressor::compress>` accepts an owning
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` and produces one independent value in the configured format.
Each call creates fresh codec state, so the same compressor can be reused for unrelated records.
Compression and decompression are unsuitable for sensitive data.
Input sensitivity flags are ignored, returned blocks are unmarked, and internal storage is not securely erased.
The input block retains its original sensitivity flag.

Most applications can simply keep the returned block.
Low-level formats sometimes need to reserve storage before compression starts; in that case
:cpp:func:`ByteCompressor::maximumCompressedLength()
<erbsland::compression::ByteCompressor::maximumCompressedLength>` provides a strict bound that includes the configured
representation's framing overhead.
The actual compressed value is usually smaller than this bound.

.. erbsland-demo::
    :source: compression/ByteCompression/CompressOneShot.cpp
    :exec: compression/byte_compression --demo CompressOneShot
    :source-sha256: 230c01e77da79e5e3e3e4b4d33844883a4e3adae15e8dd9f333da0d80707b5ca

.. code-block:: cpp

    /// Compress complete bytes using an explicitly selected algorithm, representation, and effort.
    ///
    /// The configured compressor can be reused for independent values. Its maximum
    /// compressed length includes the framing overhead of the selected format.
    void compressOneShot() {
        const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x2aU}}};
        const auto rawCompressor =
            el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Raw, el::CompressionLevel::High};
        const auto coreCompressor =
            el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core, el::CompressionLevel::High};

        // Compress one complete instrument sample in a codec stream and a Core envelope.
        const auto rawStream = rawCompressor.compress(samples);
        const auto coreEnvelope = coreCompressor.compress(samples);
        const auto maximumEnvelopeLength = coreCompressor.maximumCompressedLength(samples.length());

        el::io::printLine("Instrument        : telescópio"_el);
        el::io::printLine("Algorithm         : "_el, rawCompressor.algorithm().toString());
        el::io::printLine("Original bytes    : "_el, samples.length().toSizeT());
        el::io::printLine("Raw stream bytes  : "_el, rawStream.length().toSizeT());
        el::io::printLine("Core bytes        : "_el, coreEnvelope.length().toSizeT());
        el::io::printLine("Core upper bound  : "_el, maximumEnvelopeLength.toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : telescópio
    Algorithm         : deflate
    Original bytes    : 96
    Raw stream bytes  : 5
    Core bytes        : 41
    Core upper bound  : 138

.. erbsland-demo-end::

Compressing Input That Arrives in Pieces
========================================

Pass borrowed byte input and output streams to ``compress()``.
Deflate, bzip2, LZMA, and Zstandard retain bounded codec state while transferring a continuous representation.
Lz4Block uses capped buffering.
``CompressionTransferOptions`` configures an optional exact source prefix, buffer length, and cancellable progress
observer.
Per-I/O timeouts are configured when each stream is opened.
Borrowed streams remain open; the caller must flush or close the destination to confirm delivery.

.. erbsland-demo::
    :source: compression/ByteCompression/CompressStreaming.cpp
    :exec: compression/byte_compression --demo CompressStreaming
    :source-sha256: f3aca1474b5b2f37ef2d3a030775893da6f0b054015b7714d995a7f23a40d7f3

.. code-block:: cpp

    /// Compress instrument readings directly into a byte stream.
    void compressStreaming() {
        const auto readings = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x18U}}};
        auto source = el::stream::ByteBlockInputStream{readings};
        auto destination = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow();
        const auto compressor = el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core};
        const auto counts = compressor.compress(source, *destination);
        el::io::printLine("Instrument        : espectrômetro"_el);
        el::io::printLine("Input bytes       : "_el, counts.inputLength.toSizeT());
        el::io::printLine("Core bytes        : "_el, counts.outputLength.toSizeT());
        el::io::printLine("Native streaming  : "_el, el::BooleanFormat::yesNo(), compressor.supportsStreaming());
        if (!destination->close().isClosed()) {
            destination->abort();
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : espectrômetro
    Input bytes       : 96
    Core bytes        : 41
    Native streaming  : yes

.. erbsland-demo-end::

Decompressing Complete Values Safely
====================================

A :cpp:class:`ByteDecompressor <erbsland::compression::ByteDecompressor>` is configured with the expected algorithm,
representation, and :cpp:class:`DecompressionOptions <erbsland::compression::DecompressionOptions>`.
The options become part of the object so one-shot and streaming operations follow the same validation policy.
Each call creates fresh decoding state without changing the configuration.

``maximumOutputLength`` limits the bytes an application is willing to materialize and defaults to 256 MiB.
``maximumWorkspaceLength`` limits the decoder heap capacity reserved for windows, dictionaries, entropy tables, indexes,
and scratch buffers; its default is 64 MiB.
Caller-owned input and output, stream queues, and allocator bookkeeping have separate limits.
Codec-owned working buffers and scratch storage count toward the workspace policy.
Declared windows and dynamic reservations are checked before their allocation.

``expectedOutputLength`` is optional, but becomes valuable when a trusted container already knows the exact size.
When supplied, the decoder requires an exact match rather than treating the value as another maximum.
Raw LZ4 blocks always need this option because the block cannot announce when the reconstructed output is complete.
Other formats can usually determine their end or size themselves, though an expected length provides an additional
consistency check.

The structured error reason tells an application why decoding stopped.
Damaged headers, invalid references, checksums, and exact-length mismatches raise
:cpp:class:`CompressionError <erbsland::compression::CompressionError>`.
Its ``CompressionErrorContext`` preserves a machine-readable reason together with a concise title and optional
explanation, and ``diagnostic()`` renders these values as a human-readable error document.
The same exception reports a Core algorithm mismatch or a recognized standard feature outside the accepted stream
profile.
Output and workspace policy limits raise :cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` instead, while an
impossible constructor combination is a :cpp:class:`ParameterError <erbsland::err::ParameterError>`.
This distinction lets an application report corrupt data separately from a value it deliberately chose not to decode.

.. erbsland-demo::
    :source: compression/ByteCompression/DecompressOneShot.cpp
    :exec: compression/byte_compression --demo DecompressOneShot
    :source-sha256: 6cc1caaadaeebb41d27e45c0f3b5bb0674b1aa315a5401d0f3b48e528e89272c

.. code-block:: cpp

    /// Decompress complete raw streams and self-describing Core envelopes safely.
    ///
    /// Stored options validate an exact output length and place independent limits
    /// on returned bytes and codec workspace before decoding starts.
    void decompressOneShot() {
        const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x2aU}}};
        const auto rawBlock =
            el::ByteCompressor{el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Raw}.compress(samples);
        const auto envelope =
            el::ByteCompressor{el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Core}.compress(samples);

        // Decode a raw block with external metadata and an envelope automatically.
        const auto rawOptions = el::DecompressionOptions{}
                                    .setExpectedOutputLength(samples.length())
                                    .setMaximumOutputLength(el::ByteLength{1024U})
                                    .setMaximumWorkspaceLength(el::ByteLength{4U * 1024U * 1024U});
        const auto rawDecompressor =
            el::ByteDecompressor{el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Raw, rawOptions};
        const auto coreOptions = el::DecompressionOptions{}
                                     .setMaximumOutputLength(el::ByteLength{1024U})
                                     .setMaximumWorkspaceLength(el::ByteLength{4U * 1024U * 1024U});
        const auto coreDecompressor =
            el::ByteDecompressor{el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Core, coreOptions};
        const auto fromRaw = rawDecompressor.decompress(rawBlock);
        const auto fromEnvelope = coreDecompressor.decompress(envelope);

        el::io::printLine("Instrument        : telescópio"_el);
        el::io::printLine("Output limit      : "_el, coreDecompressor.options().maximumOutputLength().toSizeT());
        el::io::printLine("Workspace limit   : "_el, coreDecompressor.options().maximumWorkspaceLength().toSizeT());
        el::io::printLine("Raw restored      : "_el, el::BooleanFormat::yesNo(), fromRaw == samples);
        el::io::printLine("Envelope restored : "_el, el::BooleanFormat::yesNo(), fromEnvelope == samples);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : telescópio
    Output limit      : 1024
    Workspace limit   : 4194304
    Raw restored      : yes
    Envelope restored : yes

.. erbsland-demo-end::

Decompressing Input That Arrives in Pieces
==========================================

Pass borrowed byte streams to ``decompress()`` with finite output and workspace limits.
``maximumOutputLength`` limits the cumulative decoded bytes, so it can exceed the working-memory budget.
The decoder checks the complete representation before returning successfully.
Output written before a failure remains provisional; use a temporary file when incomplete output must not replace an
existing destination.

``Lz4Block`` continues to use whole-value buffering.
Select ``RequireStreaming`` when fallback buffering is unacceptable.

.. erbsland-demo::
    :source: compression/ByteCompression/DecompressStreaming.cpp
    :exec: compression/byte_compression --demo DecompressStreaming
    :source-sha256: b71ea593174b833b98f52572d36b13cd5a4d85f0efbfbbe95e06675c70428926

.. code-block:: cpp

    /// Decompress a framed instrument record directly into a byte stream.
    void decompressStreaming() {
        const auto readings = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x19U}}};
        const auto encoded =
            el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core}.compress(readings);
        auto source = el::stream::ByteBlockInputStream{encoded};
        auto destination = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow();
        const auto limits = el::DecompressionOptions{}.setMaximumOutputLength(el::ByteLength{1024U});
        const auto decompressor =
            el::ByteDecompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core, limits};
        const auto counts = decompressor.decompress(source, *destination);
        el::io::printLine("Instrument        : espectrômetro"_el);
        el::io::printLine("Core bytes        : "_el, counts.inputLength.toSizeT());
        el::io::printLine("Restored bytes    : "_el, counts.outputLength.toSizeT());
        el::io::printLine("Expected length   : "_el, el::BooleanFormat::yesNo(), counts.outputLength == readings.length());
        if (!destination->close().isClosed()) {
            destination->abort();
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : espectrômetro
    Core bytes        : 41
    Restored bytes    : 96
    Expected length   : yes

.. erbsland-demo-end::

Choosing a Practical Combination
================================

For short-lived application records where latency dominates, a raw LZ4 block inside a schema that already stores the
original length is hard to beat for simplicity.
When compatibility with long-established protocols or ZIP tools matters, Deflate is usually the most comfortable
starting point.
bzip2 can reward repetitive text with smaller results when additional CPU time is acceptable, while LZMA is attractive
for archival values whose compact size matters more than encoding speed and dictionary memory.
Zstandard provides a modern framed representation with quick decoding; its dedicated page describes the exact frame
profile accepted by this dependency-free implementation.

The surrounding representation follows a similar line of thought.
Use a standard Raw or ZIP payload when another format already carries the algorithm, boundaries, size, and integrity
metadata.
Use Core when an application-owned compressed value should carry its own algorithm identifier and lengths.
Whichever combination you choose, record it in a trusted schema and keep finite decompression limits at every boundary
where input can be influenced by somebody else.
