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
    :sha256: 1e57cd20e06bb1850de5565d22f6f4a1e101bdd7965273ce8bfd78c31a2f532a

.. code-block:: cpp

    /// Create a temporary text stream beneath an explicit directory.
    /// Temporary-file options control naming, access, and cleanup; text options independently control encoding.
    void createTemporaryText() {
        auto temporaryOptions = el::PathTempFileOptions{};
        temporaryOptions.setPrefix("θηλαστικά-"_el).setSuffix(".txt"_el).setAccessProfile(el::PathAccessProfile::UserOnly);
        auto textOptions = el::PathWriteTextOptions{el::StringEncoding::Utf8};

        const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempTextOutputStreamOrThrow(
            temporaryOptions, textOptions);
        output->writeLine("Παρατήρηση: δύο ελάφια στο ξέφωτο"_el);
        el::io::printLine("Temporary text with a .txt suffix: "_el, output->path().suffix() == ".txt"_el);
        output->close();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mTemporary text with a .txt suffix: true␛[0m

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
    :sha256: 1ecc2ffbf02540b43e7e0874eefde5e3cd43c270abf32d2423c060072fde6293

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBinary data: true␛[0m

.. erbsland-demo-end::

Remove Synchronously on Close
=============================

With ``removeOnClose()`` enabled, successful ``close()`` first finishes output and then removes the file synchronously.
After it returns ``Closed``, the path no longer exists.
This is the preferred cleanup path because removal failures can still be reported.

.. erbsland-demo::
    :source: stream/TemporaryStreams/RemoveOnClose.cpp
    :exec: stream/temporary_streams --demo RemoveOnClose
    :sha256: d65a46b82503961954952e8933454043b23b9e7e07faa226fe5dfb1d88252630

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mExisted before close: true
        Exists after close: false␛[0m

.. erbsland-demo-end::

Releasing Ownership of a Temporary File
=======================================

``release()`` disables automatic removal, empties the stream's temporary-path ownership, and returns the path.
The caller must still close the stream to finish output and then owns removal or movement of the file.
Use this for atomic publish workflows where a completed temporary file is renamed into place.

.. erbsland-demo::
    :source: stream/TemporaryStreams/ReleaseTemporaryFile.cpp
    :exec: stream/temporary_streams --demo ReleaseTemporaryFile
    :sha256: df5f152583f74998446fb337884a284c363a21daaca2636cc5c446c2116566be

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFile was retained: true
        Caller removed the file: true␛[0m

.. erbsland-demo-end::

Keep Failure Cleanup Non-Blocking
=================================

``abort()`` and destruction abandon native output immediately and schedule best-effort removal.
This keeps exception handling and shutdown bounded, but asynchronous removal errors cannot be reported to the caller.
Prefer explicit close whenever successful output matters.

.. erbsland-demo::
    :source: stream/TemporaryStreams/CleanupAfterAbort.cpp
    :exec: stream/temporary_streams --demo CleanupAfterAbort
    :sha256: bd47c9932c5bf69f8a3aed8ba38e7d7aebcfdf4ea66b3c5028a2c93c852a30d0

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAutomatic cleanup: true
        Abort returned with a closed stream: true␛[0m

.. erbsland-demo-end::
