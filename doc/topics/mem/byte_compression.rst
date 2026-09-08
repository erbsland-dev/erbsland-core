.. index::
    single: Memory; Byte compression
    single: Compression; LZ4 block
    single: Compression; Envelope
    single: ByteCompressor
    single: ByteDecompressor

****************
Compressing Data
****************

Byte compression replaces repeated patterns with a smaller representation and reconstructs the exact original bytes
later.
Erbsland Core can produce raw compressed blocks for an existing protocol or wrap them in a small self-describing
envelope.
This page shows how to choose between those representations, process complete or chunked input, and place a safe limit
on decompression.

Byte Compression and Supported Algorithms
=========================================

Lossless byte compression represents data with fewer bytes when patterns in the input can be expressed more compactly.
Decompression reconstructs the original byte sequence exactly.
Whether this saves space depends on the data and the algorithm: repeated text, structured records, and recurring byte
sequences often compress well, while very small, high-entropy, or already compressed values may stay the same size or
grow slightly because the compressed representation has its own overhead.

Compression therefore involves a practical trade-off between storage or transfer size, processing time, and memory use.
A format can always store compressed data, or it can compare the result with the original and record which
representation was chosen.
Erbsland Core's compressor always returns a valid compressed block; the surrounding format decides whether that result
is worth storing.

:cpp:class:`ByteCompressionAlgorithm <erbsland::mem::ByteCompressionAlgorithm>` selects the compression method.
Erbsland Core currently supports one algorithm:

.. list-table:: Supported compression algorithms
    :header-rows: 1
    :widths: 22 20 58

    * - Algorithm
      - Identifier
      - Best fit
    * - ``Lz4Block``
      - ``lz4-block``
      - Fast compression and especially fast decompression of independent byte blocks.

Understanding LZ4 Blocks
========================

LZ4 belongs to the LZ77 family of compression algorithms.
Instead of transforming individual byte values, it looks for sequences that already appeared in the recently processed
data.
New bytes are stored as *literals*.
When a repeated sequence is found, LZ4 stores a *match*: the distance back to an earlier occurrence and the number of
bytes to copy from there.

A raw LZ4 block is a series of these literal-and-match sequences:

.. code-block:: text

    +-------+--------------------------+----------+---------------+------------------------+
    | token | literal length extension | literals | 16-bit offset | match length extension | ...
    +-------+--------------------------+----------+---------------+------------------------+
              optional                                          optional

The one-byte token contains two four-bit values.
Its upper half begins the literal length; its lower half begins the match length, which has a minimum of four bytes.
Lengths that do not fit in these four-bit values continue in extension bytes.
After the literals, a little-endian 16-bit offset points backward from the current output position.
Offsets range from one to 65,535 bytes, giving LZ4 a window of almost 64 KiB in which to find repeated data.

For example, a sequence can store ``ABCD`` as four literals, then use offset four and match length eight to reproduce
two more copies of ``ABCD``.
The decoder already has the first copy in its output and uses it as the source for the match:

.. code-block:: text

    literals                match: offset 4, length 8
    +---------------+       +-------------------------------+
    | A | B | C | D |       | A | B | C | D | A | B | C | D |
    +---------------+       +-------------------------------+
    resulting bytes: A B C D A B C D A B C D

Matches may overlap the output currently being produced.
For example, a match with offset one can repeat the preceding byte many times, while a short repeating pattern can be
expanded from an equally short match.
The decoder mainly copies literals and earlier output bytes, and the format has no entropy-coding stage.
This simple, byte-oriented design is the reason LZ4 decompression is particularly fast.
The last sequence contains literals only and ends at the externally supplied block boundary; it has no offset or match.

LZ4 performs well when repeated sequences occur reasonably close together.
Logs, serialized records with recurring field names or values, generated resources, and data containing runs or short
patterns are typical examples.
Larger blocks generally give the compressor more opportunities to find matches, provided the useful repetition remains
within the offset window.

It is less suitable when the input is tiny, has little repetition, or has already been compressed by another format.
It also prioritizes speed over the smallest possible result, so an algorithm designed for maximum compression may be a
better choice for long-term archival data where encoding time matters less.
Repetitions separated by more than the LZ4 window cannot be referenced directly, and a raw block cannot describe its own
original size.

``Lz4Block`` uses this standard raw block representation without an LZ4 frame header.
Each Erbsland Core call creates one independent block.
The surrounding format must provide the compressed block boundary and exact original length, which the decompressor uses
to validate the reconstructed output.

The Erbsland Core Compression Envelope
======================================

An envelope carries the information missing from a raw LZ4 block.
It is useful for values stored or passed around independently, and it lets the decoder select the algorithm without
external configuration.
Its header is 24 bytes:

.. code-block:: text

    byte offset   0       4   5   6       8               16              24
                  +-------+---+---+-------+---------------+---------------+------------------+
                  | ELBC  | 1 | A | flags | original size | payload size  | compressed bytes |
                  +-------+---+---+-------+---------------+---------------+------------------+
    byte length       4     1   1     2           8               8          payload size
    byte order                                  little endian   little endian

``ELBC`` is the four-byte magic.
Version ``1`` follows, then the one-byte algorithm value ``A``.
The two flag bytes are reserved and must be zero.
Both lengths are unsigned little-endian 64-bit integers: the exact decompressed byte length and the number of bytes that
follow the header.
No trailing data is permitted.

The envelope is a compact dispatch and framing layer, not a general archive or network stream format.
Use it when an Erbsland Core value must stand alone and interoperability is under your control.
Prefer a raw block when an existing container already provides equivalent metadata, and use that format's standard
compression framing when exchanging data with other implementations.

Compressing Complete Values
===========================

Construct :cpp:class:`ByteCompressor <erbsland::mem::ByteCompressor>` with the chosen algorithm.
:cpp:func:`ByteCompressor::compress() <erbsland::mem::ByteCompressor::compress>` returns one raw block;
:cpp:func:`ByteCompressor::compressWithEnvelope() <erbsland::mem::ByteCompressor::compressWithEnvelope>` adds the
header shown above.
One-shot calls do not alter any buffered state, so a configured compressor can be reused for independent values.

Most callers can accept the returned :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` without calculating storage in
advance.
When a surrounding low-level format needs to reserve a worst-case output region,
:cpp:func:`ByteCompressionAlgorithm::maximumCompressedLength()
<erbsland::mem::ByteCompressionAlgorithm::maximumCompressedLength>` returns the required upper bound for a finite input
length.

.. erbsland-demo::
    :source: mem/ByteCompression/CompressOneShot.cpp
    :exec: mem/byte_compression --demo CompressOneShot
    :source-sha256: 2be8b9610673b98e50d501b0fcf8529f40e89a27de7a5ceac8e21cb429a78b2c

.. code-block:: cpp

    /// Compress complete bytes as a raw LZ4 block or a self-describing envelope.
    ///
    /// Raw output is compact when a surrounding format stores the algorithm and
    /// original length. An envelope carries that metadata with its payload.
    void compressOneShot() {
        const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x2aU}}};
        const auto compressor = el::ByteCompressor{el::ByteCompressionAlgorithm::Lz4Block};

        // Compress one complete instrument sample in both representations.
        const auto rawBlock = compressor.compress(samples);
        const auto envelope = compressor.compressWithEnvelope(samples);

        el::io::printLine("Instrument        : telescópio"_el);
        el::io::printLine("Algorithm         : "_el, compressor.algorithm().toString());
        el::io::printLine("Original bytes    : "_el, samples.length().toSizeT());
        el::io::printLine("Raw block bytes   : "_el, rawBlock.length().toSizeT());
        el::io::printLine("Envelope bytes    : "_el, envelope.length().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : telescópio
    Algorithm         : lz4-block
    Original bytes    : 96
    Raw block bytes   : 11
    Envelope bytes    : 35

.. erbsland-demo-end::

Compressing Input That Arrives in Pieces
========================================

:cpp:func:`ByteCompressor::update() <erbsland::mem::ByteCompressor::update>` appends each piece to the internal buffer.
Call :cpp:func:`ByteCompressor::finalize() <erbsland::mem::ByteCompressor::finalize>` for a raw block or
:cpp:func:`ByteCompressor::finalizeWithEnvelope() <erbsland::mem::ByteCompressor::finalizeWithEnvelope>` for an
envelope.
Repeated calls to the same finalizer return the cached block.

Compression happens when the value is finalized, after all input chunks have been buffered.
The API is useful when a record arrives through several callbacks but should become one compressed block.
It does not emit a sequence of compressed chunks or retain an LZ4 history across separately finalized values.
For very large or continuously produced data, divide the surrounding format into bounded blocks and compress each block
independently.

The first finalizer fixes the representation.
Updating afterward or switching between raw and envelope finalizers raises
:cpp:class:`LogicError <erbsland::err::LogicError>`.
:cpp:func:`ByteCompressor::reset() <erbsland::mem::ByteCompressor::reset>` clears buffered input and cached output so
the object can be reused.
The ``bufferedLength()`` and ``isFinalized()`` accessors are useful for observing this lifecycle.

.. erbsland-demo::
    :source: mem/ByteCompression/CompressStreaming.cpp
    :exec: mem/byte_compression --demo CompressStreaming
    :source-sha256: 69dd49b421f2e22b109f2a8a7ff57648cd40eb50acda5f643e518751e27a3d32

.. code-block:: cpp

    /// Buffer input chunks and finalize them as one compressed value.
    ///
    /// `update()` accepts pieces as they arrive. `finalizeWithEnvelope()` compresses
    /// the buffered bytes, caches the result, and prevents further updates until
    /// `reset()` is called.
    void compressStreaming() {
        const auto firstReadings = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{48U}, el::Byte{0x18U}}};
        const auto secondReadings = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{48U}, el::Byte{0x19U}}};
        auto compressor = el::ByteCompressor{el::ByteCompressionAlgorithm::Lz4Block};

        // Add two spectrometer batches, then produce one envelope.
        compressor.update(firstReadings);
        compressor.update(secondReadings);
        const auto envelope = compressor.finalizeWithEnvelope();

        el::io::printLine("Instrument        : espectrômetro"_el);
        el::io::printLine("Buffered bytes    : "_el, compressor.bufferedLength().toSizeT());
        el::io::printLine("Envelope bytes    : "_el, envelope.length().toSizeT());
        el::io::printLine("Finalized         : "_el, el::BooleanFormat::yesNo(), compressor.isFinalized());
        el::io::printLine(
            "Cached result     : "_el, el::BooleanFormat::yesNo(), compressor.finalizeWithEnvelope() == envelope);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : espectrômetro
    Buffered bytes    : 96
    Envelope bytes    : 40
    Finalized         : yes
    Cached result     : yes

.. erbsland-demo-end::

Decompressing Complete Values
=============================

A raw block needs a :cpp:class:`ByteDecompressor <erbsland::mem::ByteDecompressor>` configured with the same algorithm
and the exact original byte length passed to
:cpp:func:`ByteDecompressor::decompress() <erbsland::mem::ByteDecompressor::decompress>`.
Both pieces of metadata must come from a trusted schema or a validated surrounding container.

:cpp:func:`ByteDecompressor::decompressWithEnvelope()
<erbsland::mem::ByteDecompressor::decompressWithEnvelope>` is static because the envelope selects its own algorithm.
For untrusted input, pass the largest decompressed value the application is willing to accept.
The declared original length is compared with this limit before output storage is allocated.
Leaving the default infinite limit is reasonable only when another trusted layer already enforces a bound.

Malformed blocks, unsupported algorithms, truncated headers, and inconsistent lengths raise
:cpp:class:`ByteCompressionError <erbsland::mem::ByteCompressionError>`.
An envelope that is valid but exceeds the caller's policy limit raises
:cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>`.
Keeping these cases distinct lets an application report damaged data separately from an intentionally rejected size.

.. erbsland-demo::
    :source: mem/ByteCompression/DecompressOneShot.cpp
    :exec: mem/byte_compression --demo DecompressOneShot
    :source-sha256: c8fae324bd7c5ea44b05aeb9b2a12e42bbc73b6bca9f3534c1731112394f8311

.. code-block:: cpp

    /// Decompress complete raw blocks and self-describing envelopes.
    ///
    /// Raw decompression needs the algorithm and exact original length. Envelope
    /// decompression discovers both from the frame and can enforce an output limit
    /// before allocating the result.
    void decompressOneShot() {
        const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x2aU}}};
        const auto compressor = el::ByteCompressor{el::ByteCompressionAlgorithm::Lz4Block};
        const auto rawBlock = compressor.compress(samples);
        const auto envelope = compressor.compressWithEnvelope(samples);

        // Decode a raw block with external metadata and an envelope automatically.
        const auto rawDecompressor = el::ByteDecompressor{el::ByteCompressionAlgorithm::Lz4Block};
        const auto fromRaw = rawDecompressor.decompress(rawBlock, samples.length());
        const auto fromEnvelope = el::ByteDecompressor::decompressWithEnvelope(envelope, el::ByteLength{1024U});

        el::io::printLine("Instrument        : telescópio"_el);
        el::io::printLine("Raw restored      : "_el, el::BooleanFormat::yesNo(), fromRaw == samples);
        el::io::printLine("Envelope restored : "_el, el::BooleanFormat::yesNo(), fromEnvelope == samples);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : telescópio
    Raw restored      : yes
    Envelope restored : yes

.. erbsland-demo-end::

Decompressing Input That Arrives in Pieces
==========================================

The buffered decompressor mirrors the compressor lifecycle.
Configure an algorithm before collecting a raw block, then call
:cpp:func:`ByteDecompressor::finalize() <erbsland::mem::ByteDecompressor::finalize>` with its exact original length.
For an envelope, a default-constructed decompressor is enough; collect the chunks and call
:cpp:func:`ByteDecompressor::finalizeWithEnvelope() <erbsland::mem::ByteDecompressor::finalizeWithEnvelope>` with an
output limit.

Finalization validates and decodes the complete buffered value, then caches the result.
Call :cpp:func:`ByteDecompressor::reset() <erbsland::mem::ByteDecompressor::reset>` before collecting another value.
As with compression, this API solves chunked *input* rather than unbounded streaming output, so the chosen input and
maximum output sizes must comfortably fit in memory at the same time.

.. erbsland-demo::
    :source: mem/ByteCompression/DecompressStreaming.cpp
    :exec: mem/byte_compression --demo DecompressStreaming
    :source-sha256: 60121de3df0e3fd3b72977f63d92bf2cccc2142f88b6504e759da594a169a279

.. code-block:: cpp

    /// Buffer compressed chunks before decoding a raw block or envelope.
    ///
    /// A configured decompressor finalizes raw data with the exact output length.
    /// A default decompressor can collect an envelope and determine its algorithm
    /// and output length during finalization.
    void decompressStreaming() {
        const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x19U}}};
        const auto compressor = el::ByteCompressor{el::ByteCompressionAlgorithm::Lz4Block};
        const auto rawBlock = compressor.compress(samples);
        const auto envelope = compressor.compressWithEnvelope(samples);

        // Feed a raw block in two chunks and finalize with its external length.
        const auto rawSplit = rawBlock.length().toSizeT() / 2U;
        auto rawDecompressor = el::ByteDecompressor{el::ByteCompressionAlgorithm::Lz4Block};
        rawDecompressor.update(rawBlock.span().first(rawSplit));
        rawDecompressor.update(rawBlock.span().subspan(rawSplit));
        const auto fromRaw = rawDecompressor.finalize(samples.length());

        // Feed an envelope in two chunks and let the frame select the algorithm.
        const auto envelopeSplit = envelope.length().toSizeT() / 2U;
        auto envelopeDecompressor = el::ByteDecompressor{};
        envelopeDecompressor.update(envelope.span().first(envelopeSplit));
        envelopeDecompressor.update(envelope.span().subspan(envelopeSplit));
        const auto fromEnvelope = envelopeDecompressor.finalizeWithEnvelope(el::ByteLength{1024U});

        el::io::printLine("Instrument        : espectrômetro"_el);
        el::io::printLine("Raw restored      : "_el, el::BooleanFormat::yesNo(), fromRaw == samples);
        el::io::printLine("Envelope restored : "_el, el::BooleanFormat::yesNo(), fromEnvelope == samples);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : espectrômetro
    Raw restored      : yes
    Envelope restored : yes

.. erbsland-demo-end::

Choosing a Practical Representation
===================================

For an application-owned value without surrounding metadata, an envelope plus a finite decompression limit is the most
convenient default.
For a documented container that already records the algorithm, compressed length, and original length, a raw block
avoids redundant framing.
For large streams, define bounded independent chunks so memory use and recovery after damage remain predictable.

Keep these choices in the format specification rather than inferring them from data.
Compression ratios vary, raw LZ4 blocks are not self-describing, and every decoder still needs an application-level
maximum for the value it is willing to materialize.
