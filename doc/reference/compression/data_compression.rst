.. index::
    single: Compression
    single: Byte Compression

****************
Data Compression
****************

The byte-compression API transforms byte values and borrowed byte streams with LZ4, Deflate, bzip2, LZMA, or Zstandard.
Algorithm, representation, effort, and decompression policy are fixed when the codec object is constructed.
See
:doc:`/topics/compression/byte_compression` for format selection, codec characteristics, and examples.

Configured Compression
======================

:cpp:class:`ByteCompressor <erbsland::compression::ByteCompressor>` accepts an algorithm, a
``CompressionFormat``, and a portable ``CompressionLevel``.
``compress()`` accepts either an independent :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or a borrowed input and
output stream.
Each call creates independent codec state.
``maximumCompressedLength()`` includes the selected format's framing overhead.

:cpp:class:`ByteDecompressor <erbsland::compression::ByteDecompressor>` stores the matching algorithm and format plus
``DecompressionOptions``.
The options provide optional exact output-length validation and finite maximum output and workspace lengths.
``decompress()`` accepts a byte value or borrowed input and output streams.
The configured objects can be reused without resetting them.

Representations
===============

``Raw`` produces the codec's standard standalone payload.
``Zip`` produces the bytes for one ZIP entry's compression method, not a local header, central directory, or complete
archive.
``Core`` frames one continuous ``Raw`` payload in a version-2 ``ELBC`` envelope.
Lengths and CRC-32 follow the payload, so neither input length nor output length must be known in advance.
Core decoding requires the caller's configured algorithm to match the embedded identifier.

Errors and Resource Policies
============================

Invalid constructor combinations raise :cpp:class:`ParameterError <erbsland::err::ParameterError>`.
Malformed payloads, algorithm mismatches, unsupported stream features, and exact-length mismatches raise
:cpp:class:`CompressionError <erbsland::compression::CompressionError>`. A payload that exceeds the configured output or
workspace policy raises :cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` before the disallowed reservation.

Streaming Contract
==================

``CompressionTransferOptions`` selects an optional exact input prefix, positive buffer length (64 KiB by default),
progress observer, and whole-value fallback policy.
A finite prefix is consumed exactly; premature EOF is an error.
Without a prefix, input is consumed through EOF.
Decoders reject trailing or concatenated representations.
The returned ``CompressionTransferResult`` contains cumulative input and output lengths.
Borrowed streams remain open and are not flushed.
Accepted output can still be queued; the caller must flush or close its destination to establish delivery.

Deflate, bzip2, LZMA, and Zstandard stream natively in their supported representations.
``Lz4Block`` uses capped whole-value buffering.
``supportsStreaming()`` reports this distinction separately on each configured object.
``RequireStreaming`` rejects a fallback configuration before reading input.
Fallback input and output each have a default 256 MiB cap and may coexist with codec workspace.
One-shot methods explicitly collect a complete result and do not use this transfer fallback cap.

Progress observers receive cumulative bytes read and accepted, optional totals, and a processing, finalizing, or
completed phase.
Intermediate callbacks may return ``Cancel``.
Callback exceptions propagate.
Completion means codec validation and output acceptance succeeded, not that the destination was flushed.
Timeouts use the input/output stream settings fixed at stream creation.
A timeout terminates the transfer.
Output already accepted before an error, timeout, or cancellation remains provisional and cannot be rolled back.

``CompressionOptions`` sets compressor workspace (256 MiB by default).
Decoder workspace defaults to 64 MiB.
Total decoded output limits remain independent of working memory and must be raised explicitly for larger files.
Codec windows, block storage, indexes, and scratch storage are bounded independently of total input length.
Caller-owned storage and stream queues have separate limits.
Internal codecs use synchronous pull readers and bounded output callbacks.
They preserve decoding state across I/O chunks; they do not expose resumable sessions.
Compression and decompression APIs are unsuitable for sensitive data.
They ignore input ``ByteBlock`` sensitivity flags and return ordinary, unmarked byte blocks.
Codec buffers, dictionaries, probability models, and scratch allocations are not securely erased, including on failure.
The caller's input retains its original sensitivity flag.

Core Wire Format
================

All integer fields use little-endian order.
Version 1 is rejected.

* Header: four bytes ``ELBC``, version byte ``2``, algorithm byte, and two zero reserved bytes.
* Payload: repeated 32-bit chunk lengths followed by that many payload bytes. Valid lengths are 1 through 65,536.
* Terminator: a zero 32-bit chunk length.
* Trailer: 64-bit uncompressed length, 64-bit payload length excluding framing, and 32-bit CRC-32 of uncompressed bytes.

Writers fill 64 KiB chunks except the final chunk.
Chunk boundaries do not reset codec history.
Decoders validate framing, codec completion, cumulative lengths, CRC-32, and resource limits.

Interface
=========

.. doxygenclass:: erbsland::compression::ByteCompressor
    :members:
.. doxygenclass:: erbsland::compression::ByteDecompressor
    :members:
.. doxygenclass:: erbsland::compression::CompressionAlgorithm
    :members:
.. doxygenclass:: erbsland::compression::CompressionError
    :members:
.. doxygenclass:: erbsland::compression::CompressionErrorContext
    :members:
.. doxygenenum:: erbsland::compression::CompressionErrorReason
.. doxygenenum:: erbsland::compression::CompressionFallbackPolicy
.. doxygenenum:: erbsland::compression::CompressionFormat
.. doxygenenum:: erbsland::compression::CompressionLevel
.. doxygenclass:: erbsland::compression::CompressionOptions
    :members:
.. doxygenenum:: erbsland::compression::CompressionPhase
.. doxygenstruct:: erbsland::compression::CompressionProgress
    :members:

.. doxygentypedef:: erbsland::compression::CompressionProgressFn
.. doxygenenum:: erbsland::compression::CompressionProgressAction
.. doxygenclass:: erbsland::compression::CompressionTransferOptions
    :members:
.. doxygenstruct:: erbsland::compression::CompressionTransferResult
    :members:
.. doxygenclass:: erbsland::compression::DecompressionOptions
    :members:
