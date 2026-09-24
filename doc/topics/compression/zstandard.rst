.. index::
    single: Compression; Zstandard
    single: Zstandard; Frame
    single: Zstandard; Compression levels

****************
Zstandard Frames
****************

Zstandard, often written as zstd, is a modern format designed around fast decoding and a wide range of compression
strategies.
It places independently described blocks inside a frame that can announce its window, content size, dictionary identity,
and checksum.
That framing makes it attractive for application records and transport payloads that should carry clear decoding
boundaries.

This page explains the complete frame model before narrowing it to the self-contained profile used by Erbsland Core.
You will see how literals and repeated sequences share a compressed block, how frame fields bound memory, and what the
five portable levels mean for the frames currently produced by the dependency-free codec.

Understanding Zstandard
=======================

From Bytes to Sequences
-----------------------

Zstandard's compressed blocks combine two views of the input.
Bytes that are not part of a useful repetition become *literals*.
Repeated spans become *sequences*, each describing a literal length, a match length, and an offset into earlier output.
Recent offsets can be reused through three repeat-offset values, which makes recurring structures inexpensive to
describe.

A compressed block processes those pieces through separate entropy coders:

.. code-block:: text

    input bytes
        │
        ├───── literals ──────────── Huffman coding ───────┐
        │                                                  │
        └───── matches ─── sequences ─── FSE coding ───────┤
                                                           ▼
                                                    compressed block

Huffman coding represents literal bytes.
Finite State Entropy, or FSE, represents the sequence components using compact state transitions whose tables describe
the observed symbol frequencies.
A block may define new tables, reuse tables from an earlier block in the same frame, or use predefined forms.
The decoder interleaves decoded literals and matches to reconstruct the original order.

Not every block needs this machinery.
A *Raw_Block* stores bytes directly, which is useful when compression would not help.
An *RLE_Block* stores one byte and repeats it for the declared block size.
A *Compressed_Block* uses the literal and sequence sections above.
The fourth possible block type is reserved.

Frame and Block Layout
----------------------

A normal frame begins with the four-byte Zstandard magic number and a variable-size header.
Blocks follow until one block sets its last-block bit, after which an optional checksum completes the frame:

.. code-block:: text

    frame       +-------------+--------------+----------+----------+-----+----------+
                | 28 B5 2F FD | frame header | block 1  | block 2  | ... | checksum |
                +-------------+--------------+----------+----------+-----+----------+
                   4 bytes       variable      variable   variable         optional

    block       +------+------------+------------+-------------------------+
                | last | block type | block size | block payload           |
                +------+------------+------------+-------------------------+
                  1 bit  2 bits       21 bits      size depends on type
                └────────────── three-byte little-endian header ───────────┘

The frame-header descriptor says which optional fields are present.
A window descriptor bounds backward references when the frame is not marked as a single segment.
The content-size field, when present, lets a decoder validate or reserve the result.
A dictionary identifier refers to external shared history rather than embedding that history in the frame.
The optional four-byte content checksum contains the low 32 bits of xxHash64 over the decompressed bytes.

One family of magic numbers identifies *skippable frames* whose contents are not compressed data.
They are useful in general-purpose file streams for metadata or indexes, but an application expecting exactly one
compressed value needs an explicit policy for them and for concatenated normal frames.

Representations and the Supported Profile
-----------------------------------------

``CompressionFormat::Raw`` contains one standard Zstandard frame.
``CompressionFormat::Zip`` uses the identical frame bytes as method 93; the ZIP entry stores its CRC, sizes, and method
number around the payload.
``CompressionFormat::Core`` wraps the same standard frame in an ``ELBC`` envelope.

Erbsland Core writes self-contained frames with an explicit window descriptor, a content size when the standard header
can carry it, and a content checksum.
It currently chooses Raw blocks for general input and RLE blocks when an entire block repeats one byte.
Both are ordinary Zstandard block types, so the produced frames can be read by conforming external decoders.

The decoder accepts this single-frame Raw/RLE profile, validates the optional content size and checksum, and applies the
declared window to ``maximumWorkspaceLength`` before decoding blocks.
It reports entropy-coded Compressed blocks, external dictionary identifiers, and skippable frames as recognized but
unsupported stream features.
Concatenated frames and unrelated trailing bytes are rejected because this API returns one application value rather than
iterating over a file-level frame sequence.

These choices keep resource use easy to reason about.
Raw bytes contribute directly to the output limit, while an RLE block can expand from one payload byte to as many as 128
KiB and is checked before expansion.
The declared history window remains an independent workspace commitment even when a particular frame does not happen to
use backward matches.

The normative frame and entropy-coding rules are in `RFC 8878 <https://www.rfc-editor.org/rfc/rfc8878.html>`_.
ZIP method 93 is assigned by the `PKWARE APPNOTE <https://support.pkware.com/pkzip/appnote>`_.

Pros and Cons
=============

Where Zstandard Performs Well
-----------------------------

The standard frame makes boundaries and resource expectations visible before most decoding work begins.
Content size and checksum fields are useful for independently stored records, and the block model can move between
direct storage, runs, and full entropy coding without changing formats.
Conforming Zstandard decoders are widely suited to general application data where fast reconstruction matters.

Within Erbsland Core's current profile, long runs are represented very compactly and arbitrary bytes remain in a
portable standard frame with checksum protection.
This is useful when a project requires Zstandard framing and interoperability without adding a runtime dependency.
Core wrapping can add an application-level algorithm identity and exact payload boundary when ZIP or another container
does not already provide them.

Where Another Algorithm May Fit Better
--------------------------------------

For general repeated records, the current writer's Raw/RLE block choice does not perform the match and entropy stages
that give full Zstandard compressors their characteristic compression ratio.
Deflate, bzip2, or LZMA will usually create a smaller value from such input with the present dependency-free encoders.
LZ4 is also a better fit when a surrounding schema already provides size metadata and minimal framing is preferred.

The frame header and checksum are noticeable for tiny values.
A large declared window requires the application to permit matching workspace even if the compressed payload itself is
small.
Data exchanged with a producer that uses Compressed blocks or external dictionaries lies outside the accepted decoder
profile, so interoperability tests should use representative streams rather than only checking the magic number.

Compression Levels
==================

The five :cpp:enum:`CompressionLevel <erbsland::compression::CompressionLevel>` values reserve stable intentions for a
Zstandard encoder: from reference-like level 1 speed through level 19 compression effort.
In the current Raw/RLE writer profile, the concrete parameter that changes is the declared history window.
The decoder reads that window from the frame and never needs the portable level.

.. list-table:: Zstandard compression-level mapping
    :header-rows: 1
    :widths: 18 24 24 34

    * - Core level
      - Comparable intention
      - Declared window
      - Current block selection
    * - ``Fastest``
      - Reference level 1
      - 128 KiB
      - Raw or whole-block RLE.
    * - ``Fast``
      - Reference level 3
      - 512 KiB
      - Raw or whole-block RLE.
    * - ``Default``
      - Reference level 6
      - 2 MiB
      - Raw or whole-block RLE.
    * - ``High``
      - Reference level 12
      - 8 MiB
      - Raw or whole-block RLE.
    * - ``Highest``
      - Reference level 19
      - 32 MiB
      - Raw or whole-block RLE.

Because the current writer does not emit Compressed blocks, a higher setting does not promise a smaller payload; it
primarily changes the frame's window policy for the corresponding future-compatible effort profile.
The shared compiled demo makes this visible rather than implying a ratio that the selected data does not demonstrate.
Every frame is then decoded with a workspace limit large enough for the highest declared window.

.. erbsland-demo::
    :source: compression/ByteCompression/CompareCompressionLevels.cpp
    :function-blocks: compareCompressionLevels compareZstandardLevels
    :function-blocks-sha256: ce499797c560ded6884f7734b80b115db0530efa8644a530bda59835f493883d
    :exec: compression/byte_compression --demo CompareZstandardLevels
    :source-sha256: b3c7307dcd75e59cb0081271e8b3969dddac878977706dc3b308f7400b5dfcb7

.. code-block:: cpp

    void compareCompressionLevels(
        const el::CompressionAlgorithm algorithm,
        const el::CompressionFormat format,
        const el::StringLiteral algorithmName) {
        const auto readings = createObservatoryReadings();
        const auto options = el::DecompressionOptions{}
                                 .setExpectedOutputLength(readings.length())
                                 .setMaximumOutputLength(el::ByteLength{16U * 1024U})
                                 .setMaximumWorkspaceLength(el::ByteLength{64U * 1024U * 1024U});

        el::io::printLine("Instrument        : radiotelescópio"_el);
        el::io::printLine("Algorithm         : "_el, algorithmName);
        el::io::printLine("Original bytes    : "_el, readings.length().toSizeT());

        // Compress and restore the same observation at every portable effort level.
        for (const auto &[level, name] : cCompressionLevels) {
            const auto compressor = el::ByteCompressor{algorithm, format, level};
            const auto compressed = compressor.compress(readings);
            const auto decompressor = el::ByteDecompressor{algorithm, format, options};
            const auto restored = decompressor.decompress(compressed);
            el::io::printLine(
                name,
                " bytes: "_el,
                compressed.length().toSizeT(),
                ", restored: "_el,
                el::BooleanFormat::yesNo(),
                restored == readings);
        }
    }

    void compareZstandardLevels() {
        compareCompressionLevels(el::CompressionAlgorithm::Zstandard, el::CompressionFormat::Raw, "zstandard"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : radiotelescópio
    Algorithm         : zstandard
    Original bytes    : 3584
    Fastest bytes: 3599, restored: yes
    Fast bytes: 3599, restored: yes
    Default bytes: 3599, restored: yes
    High bytes: 3599, restored: yes
    Highest bytes: 3599, restored: yes

.. erbsland-demo-end::
