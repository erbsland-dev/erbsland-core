.. index::
    single: Compression; Deflate
    single: Deflate; RFC 1951
    single: Deflate; Huffman coding
    single: Deflate; Compression levels

***************
Deflate Streams
***************

Deflate has been carrying compressed data through ZIP files, network protocols, and application formats for decades.
Its staying power comes from a useful compromise: a modest 32 KiB history finds nearby repetition, Huffman codes make
common symbols inexpensive, and a complete stream can be decoded without a large memory commitment.

This page follows a few repeated bytes through Deflate's matching and coding stages.
You will see how stored, fixed, and dynamic blocks share one stream, why ZIP method 8 contains raw RFC 1951 bytes, and
how the portable Erbsland Core levels change the encoder's search effort.

Understanding Deflate
=====================

Finding Repeated Bytes
----------------------

The first stage is LZ77 matching.
As the compressor moves through the input, it can emit a literal byte or refer to a sequence that already appeared in
the preceding 32 KiB.
Suppose a status record contains ``TEMP=21;TEMP=21``.
The second field begins with a sequence that the decoder has just reconstructed:

.. code-block:: text

    input symbols        T E M P = 2 1 ;   T E M P = 2 1
                        └── literals ───┘ └─ length 7 ──┘
                              ▲                 │
                              └── distance 8 ───┘

Lengths range from three to 258 bytes and distances from one to 32,768 bytes.
Deflate does not write those numbers directly in most cases.
It maps them to compact length and distance symbols followed by a small number of extra bits.
Literal bytes, the special end-of-block symbol, and length symbols share one alphabet; distances use a second one.

Turning Symbols into Bits
-------------------------

Huffman coding assigns short bit patterns to common symbols and longer patterns to less common ones.
Deflate uses *canonical* Huffman trees, which means a decoder can reconstruct every code from the ordered list of code
lengths rather than receiving an explicit tree.
This saves header space and makes validation deterministic.

The stream is divided into blocks, each introduced by three bits:

.. code-block:: text

    bits on the wire        0             1 2
                           +-------------+-------+
    block header           | BFINAL      | BTYPE |
                           +-------------+-------+
                                 1 bit     2 bits, least-significant bit first

    BTYPE 00               stored bytes aligned to the next byte boundary
    BTYPE 01               literals and matches using the fixed Huffman trees
    BTYPE 10               tree description, then dynamically coded symbols
    BTYPE 11               reserved and invalid

``BFINAL`` marks the final block in the stream.
A stored block aligns to a byte boundary and carries complementary ``LEN`` and ``NLEN`` fields before its literal bytes.
It is useful for incompressible data and provides a safe upper bound.
A fixed block uses trees defined directly by RFC 1951, avoiding a per-block tree description.
A dynamic block first encodes the code lengths for its literal/length and distance trees, using a third small Huffman
alphabet to compress repeated lengths.

The whole transformation can be viewed as a short pipeline:

.. code-block:: text

    input bytes
        │
        ▼
    literals and 32 KiB backward matches
        │
        ▼
    literal/length and distance symbols
        │
        ▼
    stored bytes or canonical Huffman codes
        │
        ▼
    RFC 1951 block stream ending with BFINAL

Representations and Memory
--------------------------

``CompressionFormat::Raw`` contains only the RFC 1951 stream shown above.
There is no zlib or gzip wrapper: their checksums and headers belong to different formats.
``CompressionFormat::Zip`` contains the same bytes because a ZIP entry records method 8, sizes, and CRC outside the
compressed payload.
``CompressionFormat::Core`` wraps the Raw stream in the ``ELBC`` envelope and validates its algorithm and lengths.

Erbsland Core writes stored blocks at ``Fastest`` and fixed-Huffman blocks at the other levels, choosing the stored
result when compression would be larger.
Its decoder accepts stored, fixed-Huffman, and dynamic-Huffman blocks, so ordinary raw Deflate streams produced by other
implementations can use the format's mainstream coding choices.
One complete stream is expected: the final block must be present, padding must be valid, and trailing data is rejected.

The 32 KiB history is bounded by the format.
Previously decoded output can serve as the match source, while the Huffman tables and temporary decoding structures are
charged to the configured workspace limit.
The output limit still matters independently because a very small Deflate stream can describe a much larger repeated
value.

The normative bit-stream description is `RFC 1951 <https://www.rfc-editor.org/rfc/rfc1951.html>`_.
ZIP method 8 and its surrounding entry metadata are described by the `PKWARE APPNOTE
<https://support.pkware.com/pkzip/appnote>`_.

Pros and Cons
=============

Where Deflate Performs Well
---------------------------

Deflate is often the safest interoperability choice.
It appears in mature file formats and protocol stacks, and its 32 KiB window is small enough for constrained decoders
while still finding repeated words, markup, field names, and binary structures within typical records.
For configuration files, web-shaped text, and moderate independent payloads, it commonly gives a useful reduction
without the memory cost of a large dictionary.

Its block choices also adapt to mixed data.
An incompressible region can be stored directly, while a region with skewed symbol frequencies benefits from Huffman
coding.
The stream has an explicit final-block marker, so unlike a raw LZ4 block it does not need an externally supplied output
length merely to know where decoding ends.

Where Another Algorithm May Fit Better
--------------------------------------

The 32 KiB window cannot refer to repetitions separated by a long distance.
LZMA can use a much larger dictionary, and bzip2's block transform can group related symbols even when their original
positions are less convenient for an LZ77 match.
Modern codecs can also offer a wider speed-to-ratio range.

Deflate decoding involves bit extraction and Huffman table lookups, so LZ4 is usually a better choice when the fastest
possible reconstruction matters more than size.
At the other end of the spectrum, archival data may justify LZMA's slower but deeper search.
Very small or already compressed values can grow because even the compact block framing has a cost.

Compression Levels
==================

The five :cpp:enum:`CompressionLevel <erbsland::compression::CompressionLevel>` values map to familiar Deflate effort
points.
They influence how many candidate matches the encoder examines; the resulting RFC 1951 stream does not carry the
portable level and every conforming decoder follows the same block data.

.. list-table:: Deflate compression-level mapping
    :header-rows: 1
    :widths: 18 22 60

    * - Core level
      - Comparable effort
      - Encoder emphasis
    * - ``Fastest``
      - Level 1
      - Minimize work and use stored blocks for predictable speed.
    * - ``Fast``
      - Level 3
      - Search a short match chain before emitting fixed Huffman codes.
    * - ``Default``
      - Level 6
      - Balance match discovery with compression time.
    * - ``High``
      - Level 8
      - Examine substantially more candidate matches for a smaller stream.
    * - ``Highest``
      - Level 9
      - Use the deepest available search when compression time is secondary.

“Comparable” describes effort rather than byte-for-byte equivalence with a particular zlib release.
Different encoders may choose different valid blocks at the same familiar level.
The compiled demo below uses one observation for every portable level and then restores each stream without passing any
level to the decoder.

.. erbsland-demo::
    :source: compression/ByteCompression/CompareCompressionLevels.cpp
    :function-blocks: compareCompressionLevels compareDeflateLevels
    :function-blocks-sha256: 694ca68c5401242137af68d8a0c822e7e331ac497102851887fbb802da351c3f
    :exec: compression/byte_compression --demo CompareDeflateLevels
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

    void compareDeflateLevels() {
        compareCompressionLevels(el::CompressionAlgorithm::Deflate, el::CompressionFormat::Raw, "deflate"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : radiotelescópio
    Algorithm         : deflate
    Original bytes    : 3584
    Fastest bytes: 3589, restored: yes
    Fast bytes: 136, restored: yes
    Default bytes: 137, restored: yes
    High bytes: 50, restored: yes
    Highest bytes: 46, restored: yes

.. erbsland-demo-end::
