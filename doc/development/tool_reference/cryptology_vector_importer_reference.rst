..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************************************
Cryptology Vector Importer Reference
************************************

``tool::test_data::CryptologyVectorsTool`` imports pinned NIST, BLAKE2, Wycheproof, and x509-limbo vectors.
See :doc:`../tool_usage/import_cryptology_vectors` for online and offline workflows.

Configuration and Options
=========================

The required ``[import_cryptology_vectors]`` section is configured in
``tool/conf/tools/import_cryptology_vectors.elcl``.
Its text setting ``output_directory`` defaults to ``test/unittest/data/cryptology``.
``--output-directory`` (``-o``) overrides it.
``--check`` (``-c``) performs the same preparation without writing files or creating output directories.

``--source-directory`` selects offline mode and must name a directory inside the project.
It has no default and no corresponding configured directory.
Offline mode validates all selected files before conversion and makes no network requests.
Without this option the importer downloads sources from their configured URLs.

``--family`` accepts ``all`` (default), ``aes``, ``sha``, ``sha3``, ``gcm``, ``hmac``, ``blake2b``,
``chacha20-poly1305``, ``x25519``, ``ed25519``, ``ecdsa``, ``rsa``, and ``x509-limbo``.
Only sources belonging to the selected family are read or downloaded.

Source Records and Limits
=========================

Configuration supplies twenty-two required ``sources.<identifier>`` records, each with:

* ``url``: an absolute HTTP or HTTPS URL without credentials or a fragment.
* ``sha256``: a 64-digit hexadecimal checksum.
* ``revision``: an upstream commit, or empty for a checksum-pinned source.
* ``attribution``: a nonempty single-line upstream attribution.

All records are validated before processing.
Invalid configuration reports the offending value; invalid options report the originating option.
Source parsing failures retain their source path and underlying cause.
The source catalog fixes file names, family membership, and destination file names.

``maximum_source_bytes`` defaults to 134217728 and accepts 1 through 268435456 bytes per source.
``download_timeout_seconds`` defaults to 300 and accepts 1 through 3600 seconds per source.
Downloads are sequential and reuse a Core HTTP session.
HTTPS uses certificate authentication; redirects are limited to five and cannot downgrade HTTPS to HTTP.
There is no persistent download cache.
Each source buffer is released after conversion.

JSON parsing accepts up to 128 MiB and four million values, accommodating the approximately 41 MB limbo snapshot.
ZIP members are extracted with a 128 MiB bound.
A larger configured download limit does not raise these format limits.

Conversion and Provenance
=========================

Every source is verified before parsing.
Text sources use line-ending-independent SHA-256; ZIP archives use binary SHA-256. The shared
:ref:`text checksum contract <development-tool-framework-text-checksums>` preserves whitespace, BOM presence, and the
final line terminator.

The converter preserves the pinned revisions, reviewed selections, expected counts, and vector payloads.
Directly imported response files retain upstream content with normalized line endings and trailing horizontal
whitespace.
Converted JSON and archive records use Core ZIP, JSON, hashing, and Base-N APIs.
Negative signature and certificate payloads are preserved without production cryptology validation.

Templates under ``tool/lib/core/resources/cryptology_vectors`` own response field ordering, group iteration, conditional
sections, manifest layout, and provenance.
Native code supplies structured render values directly.
The generated ``README.rst`` records configured URLs, revisions, checksums, attribution, and selection notes.

Publication and Partial Imports
===============================

Configuration, paths, directory types, and selected destination conflicts are validated before downloads.
All outputs are prepared before any file is written.
Publication uses per-file atomic replacement; an operating-system failure during publication can leave previously
published files updated.

Unselected response files and unrelated files remain untouched.
The manifest includes selected replacements and all retained response files beneath the chosen output directory.
When ``ProjectFiles::updateText()`` retains an equivalent existing file, its actual content supplies the manifest
digest.
This also supports CRLF checkouts without special Git attributes.
See :doc:`framework_reference` for file access and replacement contracts.

Verification
============

Native tests cover each source format and family, expected selections, malformed records, negative payloads, partial
manifests, alternate outputs, configuration errors, offline isolation, and check mode.
``VectorDownloaderTest`` uses a local HTTP server for downloads, redirects, HTTP failures, deadlines, and limits.
Integration and converter tests use ``FullRun``; see
:ref:`Run the tool tests <development-tool-setup-run-the-tool-tests>`.
