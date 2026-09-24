.. index::
    single: Compression; ZIP archive
    single: ZIP64
    single: ArchiveReader
    single: ArchiveWriter

************
ZIP Archives
************

The ZIP archive API separates inspection and extraction from creation.
:cpp:class:`ArchiveReader <erbsland::compression::zip::ArchiveReader>` exposes an immutable directory and bounded
payload extraction, while :cpp:class:`ArchiveWriter <erbsland::compression::zip::ArchiveWriter>` appends entries to a
new archive and commits it only during finalization.
See :doc:`/topics/compression/working_with_zip_archives` for practical usage.

Supported Profile
=================

The reader and writer support classic ZIP and ZIP64 with Stored, Deflate, bzip2, LZMA, and Zstandard entries.
File and archive comments use UTF-8, explicit directory entries represent local subdirectories, and modification times
use both the mandatory DOS fields and the Extended Timestamp field.
ZIP64 is detected and generated automatically; ``Zip64Policy::Always`` is useful for small interoperability fixtures,
while ``Zip64Policy::Never`` rejects a record before it would need ZIP64 fields.

The accepted profile contains one disk, starts with its first local header, and ends with its central-directory end
records.
Encryption, self-extracting prefixes, archive signatures, central-directory encryption, special filesystem entries, and
multi-disk archives are rejected with a structured :cpp:class:`ZipError <erbsland::compression::zip::ZipError>`.

Specification Mapping
=====================

The implementation targets PKWARE ZIP APPNOTE 6.3.10. Local file headers and data descriptors follow sections 4.3.7 and
4.3.9; central file headers follow section 4.3.12; ZIP64 end records and locators follow sections 4.3.14 and 4.3.15; the
classic End of Central Directory follows section 4.3.16. ZIP64 extra-field substitution follows section 4.5.3, and
Extended Timestamp interpretation follows section 4.5.5.

Reader and Item Lifetimes
=========================

Reader factories take ownership of stream inputs.
Stream-backed readers require an input that supports positioning; a transferred non-positionable stream is rejected and
closed.
Directory metadata is retained in each ``ArchiveItem`` and remains readable after ``close()``.
Compressed payloads are read from the owned stream only when an item is extracted or passed to ``addItem()``, so these
operations fail after that reader closes or is destroyed.
``close()`` is idempotent.

Writer Completion
=================

New files and streams are compressed in bounded chunks.
A local header precedes the payload; a signed data descriptor records final lengths and CRC-32. ZIP64 sizes are reserved
before writing when an input length or compressed upper bound cannot prove classic fields sufficient.
``Zip64Policy::Never`` rejects such entries before emission.
Any failure after entry emission starts aborts the writer.
``finalize()`` writes the central directory, closes the stream, and is idempotent after success.
Path-backed writers use a sibling temporary file and leave an existing destination untouched until the completed archive
is moved into place.
``abort()`` terminates output immediately.
Cleanup is enabled by default; when disabled, ``temporaryPath()`` identifies retained incomplete output for recovery.

Security Limits and Validation
==============================

Reader defaults bound the central directory to 64 MiB, item count to 100,000, one extracted item to 256 MiB, codec
workspace to 64 MiB, and total bulk extraction to 1 GiB.
Writer defaults bound whole-value fallback buffers to 256 MiB and item count to 100,000. Native streaming entries may
exceed this buffering limit.

Opening reads one bounded archive tail, the fixed ZIP64 end record when present, and one bounded central-directory
block.
It validates end-record and central-directory boundaries with checked 64-bit arithmetic and rejects duplicate local
offsets, duplicate or colliding normalized paths, ambiguous entry types, unsupported flags, and trailing data after the
end record.
It does not visit local headers or payloads.
When an item is accessed, the reader validates its local metadata and optional descriptor against the central directory,
then reads only the selected compressed payload in bounded chunks.
Every extraction then verifies the exact uncompressed length and CRC-32 before committing a file.

Memory use therefore depends on the bounded directory and the currently accessed item, not the total archive length.
``extractToStream()`` borrows a destination and validates the complete entry before returning.
``extract()`` still collects a complete value within ``maximumBufferedItemLength``.
File extraction streams into a temporary file by default and commits only after successful integrity validation.
Raise total item and bulk-extraction limits explicitly when processing larger files.
Input/output stream settings configure timeouts for library-opened files; supplied streams keep their own settings.
Progress observers receive byte counts, item path, and operation phase and may cancel before completion.

Unknown extra fields are structurally validated and have no interpreted effect.
``addItem()`` can retain them with the original compressed payload; path, UTF-8, timestamp, ZIP64, and other managed
fields are regenerated for the destination archive.
Names and comments are decoded as tolerant UTF-8. Invalid sequences become replacement characters; for paths, the
decoder's U+FFFD sentinel is represented as U+FFFC so the normalized path remains usable and visibly substituted.

Interface
=========

.. doxygenclass:: erbsland::compression::zip::ArchiveDirectoryOptions
    :members:
.. doxygenclass:: erbsland::compression::zip::ArchiveEntryOptions
    :members:
.. doxygenclass:: erbsland::compression::zip::ArchiveExtractionOptions
    :members:
.. doxygenclass:: erbsland::compression::zip::ArchiveItem
    :members:
.. doxygenclass:: erbsland::compression::zip::ArchiveReader
    :members:
.. doxygenclass:: erbsland::compression::zip::ArchiveReaderOptions
    :members:
.. doxygenclass:: erbsland::compression::zip::ArchiveWriter
    :members:
.. doxygenclass:: erbsland::compression::zip::ArchiveWriterOptions
    :members:
.. doxygenenum:: erbsland::compression::zip::CompressionMethod
.. doxygenenum:: erbsland::compression::zip::Zip64Policy
.. doxygenclass:: erbsland::compression::zip::ZipError
    :members:
.. doxygenclass:: erbsland::compression::zip::ZipErrorContext
    :members:
.. doxygenenum:: erbsland::compression::zip::ZipErrorReason
.. doxygenenum:: erbsland::compression::zip::ZipOperationPhase
.. doxygenstruct:: erbsland::compression::zip::ZipProgress
    :members:

.. doxygentypedef:: erbsland::compression::zip::ZipProgressFn
