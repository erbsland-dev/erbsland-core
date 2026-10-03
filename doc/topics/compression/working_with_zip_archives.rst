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

.. erbsland-demo::
    :source: compression/ZipArchive/ReadArchive.cpp
    :exec: compression/zip_archive --demo ReadArchive
    :sha256: f73ee601835a577e06dcf48431ab1de3a429e2509a3a1035863d966bd7dcd8d5

.. code-block:: cpp

    /// Inspect validated ZIP metadata and extract selected entries under a safe root.
    void readArchive() {
        const auto workspace = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = workspace->path() / "survey.zip"_el;
        const auto northText = el::String{"north"_el};
        const auto southText = el::String{"south"_el};
        auto writer = el::zip::ArchiveWriter::create(archivePath);
        writer->addData(
            el::StringEncoder{northText}.encode(el::StringEncoding::Utf8), el::Path{"measurements/north.txt"_el});
        writer->addData(
            el::StringEncoder{southText}.encode(el::StringEncoding::Utf8), el::Path{"measurements/south.txt"_el});
        writer->finalize();

        const auto outputRoot = workspace->path() / "selected"_el;
        auto reader = el::zip::ArchiveReader::create(archivePath);
        for (const auto &item : reader->items()) {
            el::io::printLine(item->path().toString(), " : "_el, item->uncompressedLength().toSizeT(), " bytes"_el);
        }
        reader->extractToDirectory(
            outputRoot, {}, [](const el::zip::ArchiveItem &item) { return item.path().name() == "north.txt"_el; });
        reader->close();

        el::io::printLine("Extracted under: "_el, outputRoot.toString());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mmeasurements/north.txt : 5 bytes
        measurements/south.txt : 5 bytes
        Extracted under: /var/folders/f2/rq38shx1797_b7m0vy142phw0000gn/T/tmp-74W2zYT7Ijdut52gN7iB
        zhxb/selected␛[0m

.. erbsland-demo-end::

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

.. erbsland-demo::
    :source: compression/ZipArchive/WriteArchive.cpp
    :exec: compression/zip_archive --demo WriteArchive
    :sha256: 355e0f0d2fad4aa8b445f167c7e1f8ceccafaf7212cae25ebbb9db1072e78130

.. code-block:: cpp

    /// Create and finalize a ZIP archive from bytes using persistent compression defaults.
    void writeArchive() {
        const auto workspace = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = workspace->path() / "field-notes.zip"_el;
        const auto notesText = el::String{"alpine observation"_el};
        const auto notes = el::StringEncoder{notesText}.encode(el::StringEncoding::Utf8);

        auto writer = el::zip::ArchiveWriter::create(archivePath);
        writer->setCompressionMethod(el::zip::CompressionMethod::Zstandard);
        writer->setCompressionLevel(el::CompressionLevel::High);
        writer->setComment("Expedition field notes"_el);
        writer->addData(notes, el::Path{"observations/day-01.txt"_el});
        writer->finalize();

        el::io::printLine("Archive : "_el, archivePath.toString());
        el::io::printLine("Bytes   : "_el, archivePath.info().fileSize().toSizeT());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mArchive : /var/folders/f2/rq38shx1797_b7m0vy142phw0000gn/T/tmp-74W2zYT7Ijdut52gN7iBzhxb/fi
        eld-notes.zip
        Bytes   : 231␛[0m

.. erbsland-demo-end::

``finalize()`` is required because it writes the central directory and closes the owned output stream.
Repeating it after success is harmless, while every add operation then raises ``LogicFailure``.
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

Invalid caller parameters and lifecycle misuse raise ``ParameterFailure`` or ``LogicFailure``.
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
