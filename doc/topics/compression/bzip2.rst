.. index::
    single: Compression; bzip2
    single: bzip2; Burrows-Wheeler transform
    single: bzip2; Compression levels

*****************
bzip2 Compression
*****************

bzip2 approaches compression differently from LZ-style match finders.
It rearranges a sizeable block so bytes with similar surroundings gather together, turns the resulting locality into
small numbers, and finally assigns short Huffman codes to the common values.
The work is heavier than LZ4 or Deflate, but repetitive text and structured data can reward that work with compact
blocks.

This page follows that sequence from the original bytes to a complete ``BZh`` stream.
It explains the block markers and checksums a decoder relies on, the memory consequence of choosing a larger block, and
the direct relationship between Erbsland Core's compression levels and bzip2's familiar block-size digits.

Understanding bzip2
===================

Making Similar Contexts Neighbours
----------------------------------

The heart of bzip2 is the Burrows-Wheeler transform, usually shortened to BWT.
For an intuitive small example, consider the word ``BANANA`` with an end marker.
If all rotations are sorted, the final column contains the same symbols as the input but places several similar contexts
next to one another:

.. code-block:: text

    sorted rotations       first column      last column

    $ B A N A N A               $                 A
    A $ B A N A N               A                 N
    A N A $ B A N               A                 N
    A N A N A $ B               A                 B
    B A N A N A $               B                 $
    N A $ B A N A               N                 A
    N A N A $ B A               N                 A
                                                  ─────────────
                                                  A N N B $ A A

The transform itself does not discard information.
The position of the original row, together with the last column, is enough to reconstruct the original order.
A real bzip2 block uses an efficient equivalent construction rather than storing every rotation.
Its benefit becomes visible in the transformed column: symbols that occurred in similar contexts are now likely to be
close together.

The Complete Transformation
---------------------------

bzip2 surrounds the BWT with several smaller stages.
An initial run-length encoding shortens long runs before the block transform and prevents them from dominating its
working data.
After the BWT, move-to-front coding keeps an ordered alphabet: a symbol is replaced by its current position and then
moved to the front.
Recently repeated or contextually similar symbols therefore become many zeros and small integers.

Runs of zero positions are encoded through the special ``RUNA`` and ``RUNB`` symbols.
The remaining values are split into groups and written with canonical Huffman codes selected at regular intervals.
The path from input to stream looks like this:

.. code-block:: text

    input bytes
        │
        ▼
    first run-length encoding
        │
        ▼
    Burrows-Wheeler transform + original row pointer
        │
        ▼
    move-to-front positions
        │
        ▼
    RUNA/RUNB zero-run encoding
        │
        ▼
    selector-controlled canonical Huffman codes

The decoder follows the path upward.
It expands the zero runs, undoes move-to-front coding, reconstructs the original BWT row, and finally reverses the first
run-length stage.

Stream and Block Layout
-----------------------

A bzip2 payload is a complete stream rather than a bare transformed block.
It begins with ``BZh`` and an ASCII digit from ``1`` to ``9`` that announces the maximum block size in units of 100,000
bytes.
Each block and the complete stream have recognizable 48-bit markers:

.. code-block:: text

    stream       +-----+---+--------------------+-----+--------------------+---------+------------+
                 | BZh | N | block marker π...  | CRC | transformed data   |  EOS    | stream CRC |
                 +-----+---+--------------------+-----+--------------------+---------+------------+
                    3    1       6 bytes           4         variable            6          4

    block data   +------------+----------+------------+-----------+---------+---------------+
                 | randomized | BWT row  | used bytes | selectors | tables  | coded symbols |
                 +------------+----------+------------+-----------+---------+---------------+
                      1 bit      24 bits     bitmap     variable   variable     variable

The block CRC validates the bytes after all inverse transformations.
The stream CRC combines the block CRCs in order, which detects missing or rearranged blocks as well as damaged data.
Modern writers leave the historical randomized-block flag clear.

Representations and Memory
--------------------------

``CompressionFormat::Raw`` writes the complete stream, including its header, block markers, and final CRC.
``CompressionFormat::Zip`` uses the same bytes as method 12; the ZIP entry adds its own metadata outside this payload.
``CompressionFormat::Core`` wraps the Raw stream and adds Core's algorithm and length checks.

Erbsland Core accepts one complete, non-randomized bzip2 stream with ordinary Huffman groups and selectors.
Every block CRC and the combined stream CRC are verified.
The end marker must be followed only by the stream CRC and permitted final bit padding, so concatenated streams and
unrelated trailing bytes do not quietly become part of one application value.

Block size is also the main memory decision.
Undoing the BWT needs arrays that describe the sorted relationships among positions in the block, in addition to the
symbol tables and scratch space.
The decoder derives this reservation from the ``BZh`` digit and compares it with ``maximumWorkspaceLength`` before
allocating the block structures.
The returned bytes remain governed independently by ``maximumOutputLength``.

The stream and algorithm are described in the `bzip2 format documentation <https://sourceware.org/bzip2/docs.html>`_,
while ZIP method 12 is assigned by the `PKWARE APPNOTE <https://support.pkware.com/pkzip/appnote>`_.

Pros and Cons
=============

Where bzip2 Performs Well
-------------------------

The block transform is particularly effective when the input contains recurring words, tokens, or structural patterns.
Source code, logs, tabular exports, and text collections often contain similar contexts even when an exact repeated
substring is not conveniently located inside a small LZ window.
After the BWT, those contexts can produce long runs of small move-to-front values that Huffman coding represents well.

Every block is independently checksummed, and the complete stream has a second combined check.
That gives stored or transferred data a useful integrity signal before the application interprets the restored bytes.
The established stream format and ZIP method are also valuable when compatibility with existing bzip2 tooling matters.

Where Another Algorithm May Fit Better
--------------------------------------

Sorting and reconstructing a block require substantially more CPU work and temporary memory than copying LZ4 matches.
bzip2 is therefore uncomfortable on latency-sensitive paths or devices where even a 900,000-byte block and its working
arrays are too expensive.
Deflate offers broader protocol support with a smaller fixed history, while Zstandard and LZ4 are better starting points
when decode speed dominates.

The block header and tables are noticeable for tiny values.
Already compressed, encrypted, or highly random data has little structure for the BWT and entropy coder to exploit.
LZMA can also achieve a better archival ratio on data with useful long-distance repetition, although it asks for a
larger dictionary and often more encoding time.

Compression Levels
==================

bzip2's level has an unusually concrete meaning: it selects the maximum block size written in the fourth stream-header
byte.
Larger blocks give the transform more context, but also increase the workspace a decoder must permit.
The decoder reads this digit from the stream; it does not receive the portable
:cpp:enum:`CompressionLevel <erbsland::compression::CompressionLevel>` value.

.. list-table:: bzip2 compression-level mapping
    :header-rows: 1
    :widths: 18 24 58

    * - Core level
      - ``BZh`` block level
      - Maximum nominal block size
    * - ``Fastest``
      - 1
      - 100,000 bytes, favouring lower memory and shorter block work.
    * - ``Fast``
      - 3
      - 300,000 bytes, allowing more context without a large block.
    * - ``Default``
      - 6
      - 600,000 bytes, balancing context and workspace.
    * - ``High``
      - 8
      - 800,000 bytes for compression-oriented workloads.
    * - ``Highest``
      - 9
      - 900,000 bytes, the largest standard bzip2 block setting.

An input smaller than every selected block size may produce similar or identical results because all of its bytes still
fit in one block.
The following compiled example keeps the observation deliberately small, demonstrates the portable configuration, and
checks that each complete stream round-trips under explicit output and workspace limits.

.. erbsland-demo::
    :source: compression/ByteCompression/CompareCompressionLevels.cpp
    :function-blocks: compareCompressionLevels compareBzip2Levels
    :function-blocks-sha256: adf39f553e38a196ca697c4d02f11e6a48ad48e95000c2ae9b78463716b1b1cf
    :exec: compression/byte_compression --demo CompareBzip2Levels
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

    void compareBzip2Levels() {
        compareCompressionLevels(el::CompressionAlgorithm::Bzip2, el::CompressionFormat::Raw, "bzip2"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : radiotelescópio
    Algorithm         : bzip2
    Original bytes    : 3584
    Fastest bytes: 56, restored: yes
    Fast bytes: 56, restored: yes
    Default bytes: 56, restored: yes
    High bytes: 56, restored: yes
    Highest bytes: 56, restored: yes

.. erbsland-demo-end::
