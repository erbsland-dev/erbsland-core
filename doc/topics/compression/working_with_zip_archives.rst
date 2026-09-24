.. index::
    single: ZIP; Reading
    single: ZIP; Writing
    single: ZIP; Safe extraction
    single: ZIP; Reassembly

*************************
Working with ZIP Archives
*************************

ZIP archives often arrive at a trust boundary: a file name can become a filesystem path, and a small compressed item can
expand into a large output.
The ZIP API keeps those concerns visible through separate reader and writer objects, immutable item metadata, bounded
extraction, and an explicit finalization step.
This page shows how to inspect, safely extract, create, filter, and reassemble archives.

Inspecting an Archive
=====================

Open a :cpp:class:`ArchiveReader <erbsland::compression::zip::ArchiveReader>` from a path, byte block, or owned input
stream.
An owned stream must support positioning.
Before the factory returns, the reader loads and validates only the bounded archive tail and central directory; local
headers, descriptors, and payloads are deferred until the corresponding item is accessed.
An out-of-range ``item()`` lookup returns a null pointer, while ``items()`` gives you a convenient snapshot for
iteration.

Each :cpp:class:`ArchiveItem <erbsland::compression::zip::ArchiveItem>` carries a normalized relative path, an explicit
file/directory distinction, compression method, modification time, CRC-32, lengths, and comment.
Metadata remains useful after the reader closes, but payload access requires the reader because an item deliberately
keeps only a weak link to it.
Archive-level memory use is independent of the total file length, while each selected compressed and uncompressed item
is transferred in bounded chunks; whole-value extraction still returns an in-memory value.

.. code-block:: cpp

    auto reader = zip::ArchiveReader::create(archivePath);
    for (const auto &item : reader->items()) {
        io::printLine(item->path().toString(), " : "_el, item->uncompressedLength().toSizeT());
    }
    reader->close();

.. erbsland-demo::
    :source: compression/ZipArchive/ReadArchive.cpp
    :exec: compression/zip_archive --demo ReadArchive

Safe Extraction
===============

``extractToDirectory()`` combines the extraction root with each already-normalized relative item path.
The default collision mode is ``Stop``; choose ``Skip`` or ``Overwrite`` only when that behavior is part of your
application's policy.
Files are decoded and validated before an atomic sibling temporary file replaces the destination, and modification times
are preserved by default.
Existing symbolic links and reparse points below the extraction root are rejected.
During bulk extraction, explicit directories are prepared before files and their timestamps are restored after their
selected children have been written.

The reader applies a 1 GiB default total-output limit to bulk extraction and a 256 MiB default limit to individual
items.
Tune the archive-wide bounds through
:cpp:class:`ArchiveReaderOptions <erbsland::compression::zip::ArchiveReaderOptions>` and the per-operation limit through
:cpp:class:`ArchiveExtractionOptions <erbsland::compression::zip::ArchiveExtractionOptions>`.
Declared output sizes are checked before decompression; the actual decoded size and CRC-32 are checked again before
content is committed.

You can filter bulk extraction with a callback without exposing archive mutation:

.. code-block:: cpp

    reader->extractToDirectory(outputRoot, {}, [](const zip::ArchiveItem &item) {
        return !item.isDirectory() && item.path().suffix() == ".json"_el;
    });

Creating an Archive
===================

An :cpp:class:`ArchiveWriter <erbsland::compression::zip::ArchiveWriter>` writes only a new archive.
Set persistent defaults once, then add paths, byte blocks, owned input streams, or a directory tree.
Per-entry options override the compression method, level, timestamp, or comment without changing later entries.

.. code-block:: cpp

    auto writer = zip::ArchiveWriter::create(archivePath);
    writer->setCompressionMethod(zip::CompressionMethod::Zstandard);
    writer->setCompressionLevel(CompressionLevel::High);
    writer->addFile(reportPath, Path::fromPosix("reports/final.txt"_el));
    writer->addData(manifestBytes, Path::fromPosix("manifest.json"_el));
    writer->finalize();

.. erbsland-demo::
    :source: compression/ZipArchive/WriteArchive.cpp
    :exec: compression/zip_archive --demo WriteArchive

``finalize()`` is required because it writes the central directory and closes the owned output stream.
Repeating it after success is harmless, while every add operation then raises ``LogicError``.
``abort()`` is the immediate terminal alternative.
For path destinations, the old file is not replaced until the new archive is complete.

Directory Filtering
===================

:cpp:class:`ArchiveDirectoryOptions <erbsland::compression::zip::ArchiveDirectoryOptions>` controls recursive walking,
optional inclusion of the source directory itself, and include/exclude globs.
Patterns match normalized, case-sensitive paths: ``*`` and ``?`` stay within one component, while ``**`` may cross
slashes.
A metadata callback can make decisions that a name pattern cannot, and directory traversal never follows links.

ZIP64 Policy
============

``Zip64Policy::Automatic`` is the normal choice.
It writes classic records while their fields fit and substitutes ZIP64 fields when an item, offset, directory, or entry
count grows beyond a classic limit.
``Always`` makes small ZIP64 fixtures for interoperability tests.
``Never`` provides an explicit classic-only contract and fails before writing a record that requires a ZIP64 field.

Errors and Limits
=================

Invalid caller parameters and lifecycle misuse raise ``ParameterError`` or ``LogicError``.
Archive, stream, path, codec, integrity, and resource-policy failures raise
:cpp:class:`ZipError <erbsland::compression::zip::ZipError>`.
Its context includes a concise title, detailed description, machine-readable reason and operation phase, relevant
source/destination/item paths, and the retained underlying exception where one exists.

Reassembling Without Recompression
==================================

Use ``addItem()`` when an accepted entry should move from one open reader to a new writer without spending compression
work again.
The source reader validates and loads only that item's local record and compressed payload.
The writer copies the compressed payload and opaque extra fields unchanged, then regenerates managed path, UTF-8,
timestamp, ZIP64, and header fields.
This operation intentionally does not decompress the payload; ordinary extraction remains the point where CRC-32 and
uncompressed-length integrity are verified.

.. code-block:: cpp

    auto source = zip::ArchiveReader::create(sourcePath);
    auto destination = zip::ArchiveWriter::create(destinationPath);
    for (const auto &item : source->items()) {
        destination->addItem(*item);
    }
    destination->finalize();
