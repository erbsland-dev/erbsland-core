..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************************
Security Hashes Reference
*************************

``security-hashes`` generates or verifies ``security_hashes.txt`` using Core's SHA-256 and worker-pool APIs.
It has no Python runtime dependency.
For invocation examples, see :doc:`../tool_usage/security_hashes`.

Options
=======

``--verify`` and its alias ``--check`` compare without writing.
Without either flag, a changed manifest is replaced atomically.
``--jobs COUNT`` and ``-j COUNT`` accept integers from 1 through 64. The default is the logical CPU count clamped to
that interval.
Actual workers are limited to the number of selected files; empty inventories need no pool.

Configuration
=============

The required ``[security_hashes]`` section uses these fields.
All lists are optional and default to an empty selection.
ELCL accepts a single value for a value list.
Paths are project-relative POSIX paths; parent traversal, backslashes, NULs and line breaks are rejected.
Configured inputs must exist.
The output parent must already exist.

.. list-table::
    :header-rows: 1
    :widths: 30 18 52

    * - Setting
      - Type / default
      - Meaning
    * - ``output_file``
      - Text; ``security_hashes.txt``
      - Regular output file, or a new file in an existing directory.
    * - ``directories``
      - Text list; empty
      - Existing source directories scanned recursively.
    * - ``files``
      - Text list; empty
      - Existing regular files included independently of filename/suffix selection.
    * - ``included_file_names``
      - Text list; empty
      - Exact filenames accepted during recursive scanning.
    * - ``included_file_suffixes``
      - Text list; empty
      - Dotted suffixes accepted during recursive scanning.
    * - ``excluded_directory_paths``
      - Text list; empty
      - Directory subtrees pruned before scanning; missing directories are allowed.
    * - ``excluded_directory_names``
      - Text list; empty
      - Directory basenames excluded at any depth.
    * - ``excluded_file_names``
      - Text list; empty
      - Exact filenames excluded from both recursive and individual selection.
    * - ``excluded_file_suffixes``
      - Text list; empty
      - Dotted suffixes excluded from both selection methods.

Name and suffix comparisons are case-sensitive.
Exclusions override inclusion and apply to ancestors of explicitly listed files.
The output manifest always excludes itself.
Overlapping roots and repeated file selections are deduplicated.
A file selected both recursively and individually contributes to both records but is read once.

The supplied policy includes C/C++, Python, CMake, workflow YAML, ELCL, templates and dependency declarations.
It prunes ``tool/data`` and ``tool/bin`` and excludes local caches.
Compiler source templates beneath ``cmake/tools/*/data`` remain covered.
A directory with no selected direct files has no directory record.

Hash Format Version 2
=====================

All selected inputs are treated as text bytes.
The shared framework checksum converts CRLF and lone CR to LF without decoding, trimming, or removing a BOM.
Every other byte remains significant.
No timestamp cache substitutes for reading and hashing current content.

Let ``H`` mean SHA-256, ``N`` one NUL byte, and ``C(file)`` the 32-byte digest of normalized file contents.
Concatenation below is byte concatenation, with paths encoded as UTF-8:

* An individual-file digest is ``H("file" + N + relative_path + N + C(file))``.
* A directory digest is ``H("directory" + N + entries)``. Each direct-file entry is
  ``filename + N + C(file) + N``, sorted by binary UTF-8 filename order.
* The total digest is ``H("security-hashes-v2" + N + records)``. Each record is
  ``record_path + N + lowercase_hex_digest + N``, sorted by binary UTF-8 path order.

Directory record paths end in ``/``.
Paths use project-relative POSIX spelling.
The manifest begins with the generator notice, format notice, and manual-edit warning, followed by a blank line,
``* TOTAL_DIGEST``, and sorted ``PATH DIGEST`` lines.
Every line ends in LF.

Execution and Failures
======================

Configuration and options are validated before inventory.
The inventory rejects symbolic links and reparse points inside traversed trees and never follows them.
Selected inputs must be regular files.
One worker job reads and hashes each unique file.
Workers retain only their active input buffer and return fixed-size digests.
The caller assembles records after all jobs succeed, independently of scheduling order.

Verification compares the entire manifest using the same newline normalization, retaining BOM and final-newline
significance.
Generation performs that comparison before writing and leaves equivalent existing output untouched.
Changed output is committed through a sibling temporary file with project path checks before commit.
Any read or hashing failure prevents publication.
Invalid settings and options retain their source context in configuration and option diagnostics; operational failures
identify the affected input or manifest.
