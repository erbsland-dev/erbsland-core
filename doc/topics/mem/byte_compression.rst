.. index::
    single: Compression; Bytes
    single: Compression; Envelope

*****************************
Compressing and Framing Bytes
*****************************

Erbsland Core lets you compress a complete byte value as a raw LZ4 block or as a self-describing compression envelope.
This page explains how to choose the representation, enforce safe output limits, and reuse a codec for buffered input.

Choose Raw Blocks or an Envelope
================================

A raw block is useful when your surrounding file or protocol already records the algorithm and exact original size.
Construct both sides with the same :cpp:class:`ByteCompressionAlgorithm <erbsland::mem::ByteCompressionAlgorithm>` and
pass the original size when decoding.

.. code-block:: cpp

    const auto compressor = el::mem::ByteCompressor{
        el::mem::ByteCompressionAlgorithm::Lz4Block};
    const auto compressed = compressor.compress(data);

    const auto decompressor = el::mem::ByteDecompressor{
        el::mem::ByteCompressionAlgorithm::Lz4Block};
    const auto original = decompressor.decompress(compressed, data.length());

If the compressed value must stand on its own, use the Erbsland compression envelope.
It records the algorithm, original length, and compressed payload length, so decoding does not require a configured
decompressor.

.. code-block:: cpp

    const auto enveloped = compressor.compressWithEnvelope(data);
    const auto original = el::mem::ByteDecompressor::decompressWithEnvelope(enveloped);

The envelope does not authenticate the content.
Add a trusted hash or message-authentication layer when corruption or modification must be detected.

Limit Untrusted Output Before Allocation
========================================

An envelope declares its output size before its payload.
When the envelope comes from an untrusted source, pass the largest logical value your application accepts.
The decoder validates this limit before allocating output storage.

.. code-block:: cpp

    const auto maximum = el::ByteLength{1024U * 1024U};
    const auto original = el::mem::ByteDecompressor::decompressWithEnvelope(enveloped, maximum);

Malformed, truncated, unsupported, or length-inconsistent input raises
:cpp:class:`ByteCompressionError <erbsland::mem::ByteCompressionError>`.
An output limit raises :cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` so policy rejection remains
distinct from malformed compressed data.

Buffer Input in Pieces
======================

The buffered API is convenient when input arrives in several existing blocks but the caller still needs one final value.
Version 1 buffers every update and performs compression or decompression during finalization.

.. code-block:: cpp

    auto compressor = el::mem::ByteCompressor{
        el::mem::ByteCompressionAlgorithm::Lz4Block};
    compressor.update(firstPart);
    compressor.update(secondPart);
    const auto result = compressor.finalizeWithEnvelope();

The chosen finalizer fixes the representation until ``reset()``.
Repeated calls to that finalizer return the cached result, while mixing raw and envelope finalizers or updating after
finalization raises :cpp:class:`LogicError <erbsland::err::LogicError>`.
One-shot calls remain independent and do not disturb buffered input.
