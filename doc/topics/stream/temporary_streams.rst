..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Temporary Files
    single: Streams; Release Temporary File

*****************
Temporary Streams
*****************

.. erbsland-draft::

Temporary output streams combine regular byte or text output with a generated path and cleanup policy.
This page shows secure creation beneath an explicit directory, synchronous removal, ownership transfer, and abort-time
cleanup.

Create Temporary Text
=====================

Call ``Path::operations().openTempTextOutputStreamOrThrow()`` on the directory that should contain the file.
:cpp:class:`PathTempFileOptions <erbsland::path::PathTempFileOptions>` controls prefix, suffix, random-name length,
attempt count, access profile, and cleanup.
Regular ``PathWriteTextOptions`` controls encoding and output settings.

.. erbsland-demo::
    :source: stream/TemporaryStreams/CreateTemporaryText.cpp
    :exec: stream/temporary_streams --demo CreateTemporaryText
    :source-sha256: 0b8c14b944c6b9547a3657f31e2e33d128e0d564197027bc772371f1a87b496d

.. code-block:: cpp

    /// Create a temporary text stream beneath an explicit directory.
    /// Temporary-file options control naming, access, and cleanup; text options independently control encoding.
    void createTemporaryText() {
        auto temporaryOptions = el::PathTempFileOptions{};
        temporaryOptions.setPrefix("θηλαστικά-"_el)
            .setSuffix(".txt"_el)
            .setAccessProfile(el::PathAccessProfile::UserOnly);
        auto textOptions = el::PathWriteTextOptions{el::StringEncoding::Utf8};

        const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempTextOutputStreamOrThrow(
            temporaryOptions, textOptions);
        output->writeLine("Παρατήρηση: δύο ελάφια στο ξέφωτο"_el);
        el::io::printLine("Temporary text with a .txt suffix: "_el, output->path().suffix() == ".txt"_el);
        output->close();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Temporary text with a .txt suffix: true

.. erbsland-demo-end::

Create Temporary Bytes
======================

:cpp:class:`TempByteOutputStream <erbsland::stream::TempByteOutputStream>` exposes the complete byte-output API,
including endian integer helpers.
Use ``path()`` while the stream owns its generated file, but do not persist that path unless cleanup ownership is
transferred.

.. erbsland-demo::
    :source: stream/TemporaryStreams/CreateTemporaryBytes.cpp
    :exec: stream/temporary_streams --demo CreateTemporaryBytes
    :source-sha256: c60bf1e1da85edc17fbc950cf368b2344b70f3e39aa231dc40674d039949db4c

.. code-block:: cpp

    /// Temporary byte streams provide the regular atomic byte-output interface plus automatic file cleanup.
    /// Use them for intermediate binary data that should not survive the owning operation.
    void createTemporaryBytes() {
        auto options = el::PathTempFileOptions{};
        options.setPrefix("ίχνη-"_el).setSuffix(".bin"_el);
        const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow(options);
        output->writeUInt16(14U);
        output->writeUInt16(27U);
        el::io::printLine("Binary data: "_el, output->path().suffix() == ".bin"_el);
        output->close();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Binary data: true

.. erbsland-demo-end::

Remove Synchronously on Close
=============================

With ``removeOnClose()`` enabled, successful ``close()`` first finishes output and then removes the file synchronously.
After it returns ``Closed``, the path no longer exists.
This is the preferred cleanup path because removal failures can still be reported.

.. erbsland-demo::
    :source: stream/TemporaryStreams/RemoveOnClose.cpp
    :exec: stream/temporary_streams --demo RemoveOnClose
    :source-sha256: d17c945c924dbdad46e8cc00b5848a42b135324393372b23381157e0db46dea7

.. code-block:: cpp

    /// A successful temporary-stream close removes the file synchronously when `removeOnClose()` is enabled.
    /// This gives the caller a precise point after which the path no longer exists.
    void removeOnClose() {
        const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempTextOutputStreamOrThrow();
        const auto path = output->path();
        output->writeLine("Λύγκας"_el);
        el::io::printLine("Existed before close: "_el, path.info().exists());
        output->close();
        el::io::printLine("Exists after close: "_el, path.info().exists());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Existed before close: true
    Exists after close: false

.. erbsland-demo-end::

Release a File Deliberately
===========================

``release()`` disables automatic removal, empties the stream's temporary-path ownership, and returns the path.
The caller must still close the stream to finish output and then owns removal or movement of the file.
Use this for atomic publish workflows where a completed temporary file is renamed into place.

.. erbsland-demo::
    :source: stream/TemporaryStreams/ReleaseTemporaryFile.cpp
    :exec: stream/temporary_streams --demo ReleaseTemporaryFile
    :source-sha256: 0fa278230664cd11693362e285a381876f4e60df718d692efaad194ced3682b9

.. code-block:: cpp

    /// Call `release()` when the generated file must outlive the temporary stream.
    /// Releasing transfers cleanup responsibility to the caller; close the stream and remove the returned path explicitly.
    void releaseTemporaryFile() {
        const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempTextOutputStreamOrThrow();
        output->writeLine("Καταγραφή αρκούδας"_el);
        const auto retainedPath = output->release();
        output->close();

        el::io::printLine("File was retained: "_el, retainedPath.info().exists());
        retainedPath.operations().removeOrThrow();
        el::io::printLine("Caller removed the file: "_el, !retainedPath.info().exists());
    }

.. erbsland-ansi::
    :escape-char: ␛

    File was retained: true
    Caller removed the file: true

.. erbsland-demo-end::

Keep Failure Cleanup Non-Blocking
=================================

``abort()`` and destruction abandon native output immediately and schedule best-effort removal.
This keeps exception handling and shutdown bounded, but asynchronous removal errors cannot be reported to the caller.
Prefer explicit close whenever successful output matters.

.. erbsland-demo::
    :source: stream/TemporaryStreams/CleanupAfterAbort.cpp
    :exec: stream/temporary_streams --demo CleanupAfterAbort
    :source-sha256: 1bc50e9d991841d3b40ba0e814026319dfe38e0dbe39f2302320657cdc76489f

.. code-block:: cpp

    /// Aborting a temporary stream returns immediately and schedules removal when automatic cleanup is enabled.
    /// Destruction uses the same fallback, which keeps exception paths and shutdown from waiting on native I/O.
    void cleanupAfterAbort() {
        const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow();
        output->writeUInt32(0x4d414d4dU);
        el::io::printLine("Automatic cleanup: "_el, output->removeOnClose());
        output->abort();
        el::io::printLine("Abort returned with a closed stream: "_el, output->state() == el::StreamState::Closed);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Automatic cleanup: true
    Abort returned with a closed stream: true

.. erbsland-demo-end::
