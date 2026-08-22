.. index::
    single: Byte Compression
    single: LZ4

****************
Byte Compression
****************

The byte-compression API provides raw LZ4 blocks and a framed representation that records the algorithm and original
length.
Use raw blocks when another format already carries this metadata, and use the envelope for standalone stored or
transmitted values.

Raw Compression
===============

:cpp:class:`ByteCompressor <erbsland::mem::ByteCompressor>` always returns a valid raw block, even when the block is
larger than its input.
Raw :cpp:class:`ByteDecompressor <erbsland::mem::ByteDecompressor>` calls require the exact original size and reject
output-length mismatches.

Compression Envelopes
=====================

``compressWithEnvelope()`` frames the raw payload with the stable ``ELBC`` header, algorithm identifier, original
length, and payload length.
:cpp:func:`ByteDecompressor::decompressWithEnvelope()
<erbsland::mem::ByteDecompressor::decompressWithEnvelope>` validates the complete frame and automatically selects the
encoded algorithm.
The optional maximum-output limit is checked before allocating output storage.
The envelope provides framing only; it is not an integrity or authentication mechanism.

Buffered Operation
==================

Both codec classes accept input through ``update()`` and emit one result during finalization.
The first successful finalizer selects raw or envelope output until ``reset()`` is called, and repeated calls to the
same finalizer return the cached result.
One-shot methods do not alter this buffered state.

Interface
=========

.. doxygenclass:: erbsland::mem::ByteCompressionAlgorithm
    :members:
.. doxygenclass:: erbsland::mem::ByteCompressionError
    :members:
.. doxygenenum:: erbsland::mem::ByteCompressionErrorReason
.. doxygenclass:: erbsland::mem::ByteCompressor
    :members:
.. doxygenclass:: erbsland::mem::ByteDecompressor
    :members:
