.. index::
    single: Compression; LZMA
    single: LZMA; LZMA-Alone
    single: LZMA; Range coding
    single: LZMA; Compression levels

****************
LZMA Compression
****************

LZMA is built for situations where spending more time and memory during compression can earn a smaller stored value.
It combines a large LZ77 dictionary with an adaptive probability model and a range coder, allowing it to describe both
long-distance repetition and the changing statistical character of the data.
That combination is a strong fit for archives, software packages, and large structured resources.

This page introduces LZMA1 from the viewpoint of a reader who needs to choose and safely decode its representation.
It explains matches and repeated distances, shows how the properties and dictionary size appear in LZMA-Alone and ZIP
method 14, and connects the five portable compression levels to concrete dictionary and search choices.

Understanding LZMA
==================

Matches, Repetitions, and State
-------------------------------

Like Deflate, LZMA begins by looking backward for bytes that have already appeared.
Its dictionary can be much larger than Deflate's 32 KiB window, so a later record can refer to an earlier pattern that
would otherwise be out of reach.
For a long observation containing the same calibration header every few megabytes, that wider history can be valuable.

LZMA does not treat every match as an unrelated length and distance.
It remembers the four most recent distances and can describe a *repetition* of one of them with fewer decisions.
It also keeps a small state describing whether recent symbols were literals, new matches, or repeated matches.
That state changes the probabilities used for the next decision:

.. code-block:: text

    input bytes
        │
        ▼
    literal, new match, or repeated-distance decision
        │
        ├──────── literal context from position and previous byte
        │
        └──────── match length and dictionary distance
        │
        ▼
    adaptive binary probability models
        │
        ▼
    range-coded byte stream

Literal bytes have their own context model.
The standard properties ``lc`` and ``lp`` select how many high bits of the preceding byte and low bits of the current
position contribute to that context.
``pb`` selects the position-state bits used by match and length decisions.
These values let one LZMA1 stream adapt to text-like data, aligned binary records, and other recurring structures.

Range Coding
------------

Most LZMA decisions are binary: literal or match, repeated or new distance, one length branch or another.
Each decision has a probability that moves gradually toward the symbols actually observed.
A range coder turns the sequence of probability-weighted decisions into one fractional interval and emits bytes as the
leading part of that interval becomes stable.

Unlike Huffman coding, a likely decision is not forced to consume a whole number of bits.
Many strongly predicted decisions can therefore be represented very compactly.
The price is more arithmetic and more model state during both compression and decompression.
The models begin from known initial probabilities, so the decoder rebuilds them by making exactly the same decisions in
the same order; they do not need a large table in the stream header.

Headers and Representations
---------------------------

LZMA1 is the compressed coding method.
LZMA-Alone adds a small header that makes one stream independently decodable:

.. code-block:: text

    LZMA-Alone    +------------+-----------------+-------------------+-------------------+
                  | properties | dictionary size | uncompressed size | range-coded data  |
                  +------------+-----------------+-------------------+-------------------+
                    1 byte       4 bytes           8 bytes             variable
                                 little endian     little endian

The properties byte packs ``lc``, ``lp``, and ``pb`` into one value.
The dictionary field tells the decoder how far a match may reach and therefore how much history it must be prepared to
maintain.
The eight-byte length is either an exact size or the all-ones value used for an unknown size terminated by the LZMA end
marker.

``CompressionFormat::Raw`` uses this complete LZMA-Alone representation and writes the exact output size.
``CompressionFormat::Core`` wraps the same bytes, adding Core's independent algorithm and payload validation.
ZIP method 14 replaces the Alone header because ZIP already stores the uncompressed size in its entry metadata:

.. code-block:: text

    ZIP method 14 +---------------+-----------------+------------+-----------------+-------------------+
                  | LZMA version  | property length | properties | dictionary size | range-coded data  |
                  +---------------+-----------------+------------+-----------------+-------------------+
                    2 bytes          2 bytes          1 byte       4 bytes           variable
                    little endian    little endian                 little endian

``CompressionFormat::Zip`` produces only this method-14 payload, not the ZIP entry around it.
The stream includes an LZMA end marker so the payload is self-terminating within its known ZIP boundary.

Representations and Memory
--------------------------

Erbsland Core writes the common LZMA1 properties ``lc=3``, ``lp=0``, and ``pb=2`` and accepts valid properties whose
literal-context combination fits its bounded model workspace.
It accepts an exact LZMA-Alone size or an end-marked stream, and it accepts the standard ZIP method-14 header profile
written by the compressor.
Every match must remain inside the declared dictionary and already reconstructed output; incomplete range-coded values
and unrelated trailing bytes are rejected.

The dictionary size is both a compression opportunity and a decoder commitment.
Before range decoding begins, the declared dictionary and fixed probability model are checked against
``maximumWorkspaceLength``.
This prevents a tiny stream header from requesting an allocation beyond the application's policy.
The exact or discovered output remains subject to ``maximumOutputLength``, and an optional expected length must agree
with the LZMA-Alone header when both are present.

The LZMA1 coding method and properties are documented by the `LZMA SDK <https://www.7-zip.org/sdk.html>`_.
The method-14 payload header is specified by the `PKWARE APPNOTE <https://support.pkware.com/pkzip/appnote>`_.

Pros and Cons
=============

Where LZMA Performs Well
------------------------

LZMA is most compelling when data will be compressed occasionally and stored or distributed many times.
Large source trees, application resources, database exports, and installation packages often repeat meaningful sequences
across distances larger than Deflate can reach.
The adaptive literal model also benefits data whose local byte patterns are predictable even when an exact long match is
unavailable.

A range coder can represent highly probable choices more finely than a Huffman code, which helps LZMA achieve strong
ratios on suitable data.
The LZMA-Alone header records the properties, dictionary, and expected result size in only 13 bytes, while ZIP method 14
fits the same LZMA1 coding into an established archive entry.

Where Another Algorithm May Fit Better
--------------------------------------

Deep match searches make compression slower, and a large dictionary becomes a real workspace requirement for the
decoder.
That trade-off is often inappropriate for interactive messages, short-lived caches, or small embedded targets.
LZ4 offers much quicker decoding with tiny codec workspace, while Deflate provides mature compatibility with a fixed 32
KiB history.

Small values cannot make good use of a multi-megabyte dictionary, yet still carry the properties and length header.
Encrypted, random, and already compressed data gives the match finder and probability models little useful structure.
Zstandard is often a more balanced choice when applications want a modern frame and fast decoding without pursuing the
smallest archival result.

Compression Levels
==================

For LZMA, :cpp:enum:`CompressionLevel <erbsland::compression::CompressionLevel>` selects both the dictionary advertised
in the stream and the depth of the encoder's match search.
The dictionary is part of the format because the decoder must enforce match distances; the portable level itself is not
stored and is not needed for decompression.

.. list-table:: LZMA compression-level mapping
    :header-rows: 1
    :widths: 18 24 20 38

    * - Core level
      - Dictionary
      - Search depth
      - Intended balance
    * - ``Fastest``
      - 256 KiB
      - 4 candidates
      - Short searches and modest decoder workspace.
    * - ``Fast``
      - 2 MiB
      - 16 candidates
      - More reach while keeping compression responsive.
    * - ``Default``
      - 8 MiB
      - 64 candidates
      - General archival compression with bounded memory.
    * - ``High``
      - 16 MiB
      - 128 candidates
      - Prefer size and longer-distance matches.
    * - ``Highest``
      - 32 MiB
      - 256 candidates
      - Deepest search and largest supported dictionary.

The decoder's workspace limit must accommodate the dictionary selected by the encoder.
Increasing the level cannot help when the input is smaller than the lower dictionary or contains no additional useful
matches, so measure representative data before paying the highest compression cost.
This compiled demo applies every level to the same observation and restores each LZMA-Alone stream with one bounded
decoder configuration.

.. erbsland-demo::
    :source: compression/ByteCompression/CompareCompressionLevels.cpp
    :function-blocks: compareCompressionLevels compareLzmaLevels
    :function-blocks-sha256: 354eb492472b05496052b930785e5819b9907516f2cf9116b2d25470fe662e8a
    :exec: compression/byte_compression --demo CompareLzmaLevels
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

    void compareLzmaLevels() {
        compareCompressionLevels(el::CompressionAlgorithm::Lzma, el::CompressionFormat::Raw, "lzma"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument        : radiotelescópio
    Algorithm         : lzma
    Original bytes    : 3584
    Fastest bytes: 121, restored: yes
    Fast bytes: 125, restored: yes
    Default bytes: 66, restored: yes
    High bytes: 66, restored: yes
    Highest bytes: 61, restored: yes

.. erbsland-demo-end::
