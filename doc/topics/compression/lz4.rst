.. index::
    single: Compression; LZ4
    single: LZ4; Raw block
    single: LZ4; Compression levels

**********
LZ4 Blocks
**********

LZ4 is designed for the moments when decompression should feel almost invisible.
It replaces nearby repeated byte sequences with short backward references and otherwise leaves bytes as literals.
There is no statistical model to rebuild and no entropy-coded bit stream to navigate, so a decoder can spend most of its
time copying bytes.

This page builds a raw LZ4 block from a small repeated pattern, follows its fields on the wire, and explains why the
algorithm is such a good fit for independent application records.
It also shows where LZ4 gives up compression ratio in exchange for speed and how Erbsland Core's five portable levels
change the match search.

Understanding LZ4
=================

From Literals to Matches
------------------------

Imagine that an instrument produces the bytes ``ABCDABCDABCD``.
The first ``ABCD`` has not appeared before, so the compressor writes those four bytes as *literals*.
When the next ``ABCD`` begins, the same sequence is only four bytes behind the current position.
Instead of writing eight more literal bytes, the compressor can describe one *match* with distance four and length
eight:

.. code-block:: text

    input                        A B C D         A B C D A B C D
                                └── literals ──┘└──── match ────┘

    decoded output               A B C D       A B C D  A B C D
                                └── source ──┘└─ copied twice ─┘
                                      ▲               │
                                      └─── offset 4 ──┘

The source and destination are allowed to overlap.
After the decoder copies the first four match bytes, those newly produced bytes become part of the source for the next
four.
This small rule also makes long runs inexpensive: a match with offset one can repeat the preceding byte many times.

Every match must be at least four bytes long.
Shorter repetitions would rarely save space after their length and offset were recorded.
The offset is an unsigned 16-bit value, so a match can refer to bytes between one and 65,535 positions behind the
current output.
Useful repetition beyond that window must be written literally or found again closer to the current position.

Sequences and Tokens
--------------------

A raw block is a series of *sequences*.
Each regular sequence contains some literals followed by one match; the final sequence contains literals only.
The one-byte token keeps common lengths compact by dividing itself into two four-bit fields:

.. code-block:: text

    bit              7             4 3             0
                    +----------------+---------------+
    token           | literal length | match length  |
                    +----------------+---------------+
                       high nibble      low nibble

    sequence        +-------+-------------+----------+--------+--------------+
                    | token | lit. length | literals | offset | match length |
                    +-------+-------------+----------+--------+--------------+
                                optional              optional
    byte order                                                 little endian

The upper nibble starts the literal length.
Values below 15 are complete; a value of 15 means extension bytes follow before the literals.
Each extension byte contributes its value, and a byte containing 255 means another extension byte follows.
The lower nibble describes the match length in the same way, except that four is added because every match already has
the minimum length of four.

After the literals, the two-byte little-endian offset points backward from the current output position.
An offset of zero is invalid.
The final sequence stops after its literals, so it has neither an offset nor a match-length extension.
The end of the compressed block must come from the surrounding container; the raw format has no end marker, output size,
checksum, or frame header.

Representations and Memory
--------------------------

``CompressionFormat::Raw`` produces exactly this standard block representation.
Because the decoder cannot learn the original size from the block, a raw LZ4
:cpp:class:`ByteDecompressor <erbsland::compression::ByteDecompressor>` requires
``DecompressionOptions::expectedOutputLength``.
That value normally comes from a trusted record header or schema and is checked exactly.

``CompressionFormat::Core`` places the block inside the ``ELBC`` envelope described on the
:doc:`main compression page <byte_compression>`.
The envelope supplies the algorithm identifier, original length, and compressed boundary that the raw block omits.
``CompressionFormat::Zip`` is not available for LZ4 because this API does not define a ZIP method for raw LZ4 blocks.

Decoding needs no separate dictionary allocation: previously produced output is the match history.
This keeps codec workspace small, although the compressed input and returned output naturally still occupy memory.
The decoder validates every offset and length before copying, rejects trailing data, and requires the last sequence to
end exactly at the supplied block boundary.

The raw layout is defined by the `LZ4 Block Format Description
<https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md>`_.

Pros and Cons
=============

Where LZ4 Performs Well
-----------------------

LZ4 is a natural companion for caches, network messages, generated resources, and serialized records that are decoded
often or on a latency-sensitive path.
Logs and structured binary data tend to repeat field names, tags, padding, and common values within the 64 KiB match
window.
The decoder turns those matches back into bytes with straightforward copies, which is especially attractive when one
piece of data is written once but read many times.

Independent blocks also give an application useful recovery boundaries.
A damaged record does not require rebuilding a large statistical state from everything that came before it.
With Core framing, the application receives this speed while also getting an exact size and a clear algorithm identity.

Where Another Algorithm May Fit Better
--------------------------------------

The same simplicity limits LZ4's compression ratio.
It does not assign shorter codes to common literal values, and it cannot reference repetitions more than 65,535 bytes
away.
Deflate adds Huffman coding, while bzip2, LZMA, and Zstandard use richer transformations or models that can describe
some data more compactly.

Tiny values can grow because even a token and length information have a cost.
Random, encrypted, and already compressed bytes usually have no useful matches at all.
For long-term archives, where encoding happens rarely and storage cost dominates, LZMA or another deeper compressor may
justify its additional work.
Raw LZ4 is also a poor fit when the surrounding format cannot provide an exact original length.

Compression Levels
==================

:cpp:enum:`CompressionLevel <erbsland::compression::CompressionLevel>` controls how much work the compressor spends
looking for a useful match at each position.
The resulting bytes do not record this abstract level, and the decoder does not need to know it.
All levels produce the same standard raw block representation.

.. list-table:: LZ4 compression-level mapping
    :header-rows: 1
    :widths: 18 24 58

    * - Core level
      - Match-chain depth
      - Search behaviour
    * - ``Fastest``
      - 1 candidate
      - Accept the first useful nearby match and minimize search work.
    * - ``Fast``
      - Up to 4 candidates
      - Spend a little more time comparing recent alternatives.
    * - ``Default``
      - Up to 16 candidates
      - Balance match quality and compression time for general records.
    * - ``High``
      - Up to 64 candidates
      - Search more deeply and look ahead one byte before committing to a match.
    * - ``Highest``
      - Up to 256 candidates
      - Use the deepest search and the same lazy-match decision for compact output.

A deeper search can find a longer or closer repetition, but it is not a promise that every higher level produces fewer
bytes.
The best result still depends on the input, and a small value may offer every level exactly the same useful match.
The following compiled demo compresses one observation at all five levels and verifies every result with a decoder that
knows only the algorithm, representation, and expected output length.

.. erbsland-demo::
    :source: compression/ByteCompression/CompareCompressionLevels.cpp
    :function-blocks: compareCompressionLevels compareLz4Levels
    :function-blocks-sha256: 9e9638a3b8ff0e6e5297ce64407cedadb058c3458624269a9715fed8e083107f
    :exec: compression/byte_compression --demo CompareLz4Levels
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

    void compareLz4Levels() {
        compareCompressionLevels(el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Raw, "lz4-block"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : radiotelescópio
    Algorithm         : lz4-block
    Original bytes    : 3584
    Fastest bytes: 58, restored: yes
    Fast bytes: 41, restored: yes
    Default bytes: 41, restored: yes
    High bytes: 41, restored: yes
    Highest bytes: 41, restored: yes

.. erbsland-demo-end::
