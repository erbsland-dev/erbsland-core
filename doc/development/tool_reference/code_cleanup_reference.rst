..
    Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********************
Code Cleanup Reference
**********************

``tool::cleanup`` implements the ``code_cleanup`` configuration section and ``code-cleanup`` command.
For invocation examples, see :doc:`../tool_usage/code_cleanup`.

Configuration
=============

The shipped policy is ``tool/conf/tools/code_cleanup.elcl``.
Filesystem paths are project-relative and confined through the framework's ``ProjectFiles`` service.
Directory globs must match existing directories; walks skip symlinks and reparse points.

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Setting
      - Meaning and default
    * - ``Library Directory``
      - Implementation root for resolution; ``src/erbsland``.
    * - ``Public Include Prefix``
      - Compiler-visible directory before mapped header names; ``erbsland``. Nested prefixes such as ``vendor/core`` are allowed.
    * - ``Public Header Map``
      - Canonical-first output of ``update-public-headers``; ``tool/data/public_headers/header_map.txt``.
    * - ``File Suffixes``
      - Required list of dotted suffixes; shipped: ``.hpp``, ``.h``, ``.hxx``, ``.tpp``, ``.cpp``.
    * - ``Excluded Names``
      - Optional exact filenames excluded from transformation; shipped: ``fwd.hpp``, ``all.hpp``.
    * - ``Sources.<name>.Directories``
      - Required list of project-relative directory globs; matched directories are scanned recursively.
    * - ``Sources.<name>.Include Style``
      - ``public`` by default; ``relative`` requires the library implementation root.
    * - ``Sources.<name>.Include Roots``
      - Optional extra local header search roots scoped to this tree.
    * - ``CMake.Minimum Version``
      - Version written in simple CMake files; ``3.28``.
    * - ``CMake.Trees.<name>.Directory`` / ``Target``
      - Explicit recursive CMake source-list target; the deepest matching directory wins.
    * - ``Format.Executable``
      - ``clang-format`` on PATH, or an absolute executable path outside the project.
    * - ``Format.Style File``
      - One style file for all sources; ``.clang-format``.
    * - ``Format.Cache File``
      - Disposable cache; ``.cache/code_cleanup/fingerprints.txt``.

Set ``Shared Include Prefix`` for an extension that shares its public prefix with Core.
Cleanup preserves external angle includes that are absent from the extension's header map and source tree.

Generated CMake headers use the framework's shared ``Rendering`` copyright and license settings.
The default copyright spelling is ``Copyright (c)``.
Cleanup preserves existing copyright notices in C++ sources; their generators own those headers.
The shipped C++ scope covers library sources, unit tests, demos, constant-time and profiling tools, adapters, build
helpers, fuzzers, interoperability and heap tests, macOS tests, CMake fixtures, and maintenance tool sources, tests, and
test helpers.
Selections use actual source directories so sibling data and generator templates are preserved.
CMake cleanup covers ``src``, ``test/unittest/src``, ``tool/app/src``, ``tool/lib/*/src``, and
``tool/test/unittest/src``.

``Directories`` accepts plain paths, ``*`` within a path component, and recursive ``**`` patterns.
Each pattern matches complete directory paths and must select at least one directory.
Empty, malformed, absolute, and parent-traversing patterns are rejected.
Selection is resolved on every configuration read in deterministic order without following links.
Duplicate and nested roots within one named policy collapse to one scan; roots from different policies must not overlap.
Each expanded tree automatically uses its own root for local includes, plus that policy's literal ``Include Roots``.
Relative includes into an explicit include root retain their spelling, even when the root lies outside the source tree.
Include roots do not select additional files for cleanup.
The library's relative-include policy must still select only ``Library Directory``.
CMake tree ``Directory`` settings remain literal because each mapping names one explicit build target.

Include Repair and Layout
=========================

The leading region contains ordinary line comments, blank lines, ``#pragma once``, and include directives.
API documentation comments, ``clang-format off``, other directives, continued directives, or C++ code end that region.
Conditional includes and trailing template includes remain outside path repair and sorting.
Standalone comments and macro includes form ordering barriers; literal includes never cross them.
Trailing comments remain attached to their include.
Only identical complete include lines deduplicate, preserving different annotations and case-sensitive paths.

``Library Directory`` determines the source prefix used to recognize project includes; no source directory name is
hard-coded in the resolver.
``Public Include Prefix`` is independent of that physical directory and must be a nonempty slash-separated include
directory without ``.`` or ``..`` components.
Mapping source names are relative to ``Library Directory``; mapping destination names are relative to
``Public Include Prefix``.
For example, ``Library Directory: "vendor/base/source"`` and ``Public Include Prefix: "vendor/base"`` allow
``vendor/base/source/a/Widget.hpp`` to become ``<vendor/base/Widget.hpp>`` when the map contains
``a/Widget.hpp;Widget.hpp``.

Existing relative library targets resolve first.
Consumer-local headers and their ``.in`` templates then take precedence over guessing a Core header.
Library resolution uses exact source paths, unique path suffixes, and finally filenames with a unique closest library
context.
Ambiguities and missing quoted project headers produce contextual errors.
Canonical mapping preserves merged public wrappers, internal includes, and unrelated Erbsland libraries.

Includes use one case-insensitive comparison; equal folded spellings retain their original order.
Groups are the corresponding source header, local filenames, local subdirectories, one-level parent paths, deeper parent
paths, angle includes ending in ``.h`` or ``.hpp``, extensionless angle includes, and remaining angle includes.
Each group is separated by one blank line.
The corresponding header also matches ``Class_part.cpp`` to ``Class.hpp``.

The cheap layout pass removes the gap between the copyright comments and ``#pragma once``, leaves a blank line after
``#pragma once``, inserts blank lines around namespace blocks, and separates namespace-scope function definitions in
``.cpp`` files.
Core's C++ tokenizer distinguishes comments, literals, raw strings, and complete preprocessor directives.
Manual ``clang-format off`` blocks and compact empty namespace declarations retain their spacing.
It is a structural scanner, not a C++ compiler or a general-purpose formatter.

Simple CMake Grammar
====================

The managed prefix accepts the simple ``cmake_minimum_required(VERSION number)`` form, single-argument
``add_subdirectory``, and plain ``target_sources(target PRIVATE ...)`` commands.
Subdirectories sort and deduplicate; source lists merge, sort, and gain unlisted local source files.
A source referenced in a recognized conditional block is not added unconditionally.
Conditional source lists retain their indentation and surrounding conditional text.

Files containing unsupported commands or argument syntax are preserved.
Examples include public/interface visibility, expressions, quoted arguments, multiple targets, annotated source lists,
and additional ``add_subdirectory`` arguments such as ``EXCLUDE_FROM_ALL``.
Missing directory-local files or subdirectories in an otherwise supported file are errors.
The tool does not create CMake files or infer new subdirectory commands.

Smart Formatting and Failures
=============================

One inventory collects regular files and builds read-only header and local-source indexes.
One Core worker pool reads and transforms independent files with ordered results.
Jobs share immutable source policies through managed pointers.
Text helpers use COW ``StringList`` values; the spacing scanner owns per-file state and tokenizes only when its cheap
candidate scan finds a potential gap.
Native token byte positions allow a single forward traversal of the physical lines, without counting code points or
maintaining a line-offset search table.
CMake commands are parsed and validated once using cached expressions, then rendered from the per-file document.
``--jobs`` accepts 1 through 64; the default is the logical CPU count clamped to that range.
Workers never wait on jobs submitted to the same pool.

Cheap cleanup runs first in memory.
The formatter cache compares SHA-256 fingerprints of the remaining code after removing the leading comment, pragma, and
include region.
A selected file is formatted to captured stdout without ``-i``, then passed through cheap cleanup again to compensate
for clang-format's spacing gaps and normalize paths after its include reordering.

The first run formats all files, including Git-clean files.
A missing, incompatible, or invalid cache also requires a new full baseline.
The cache signature includes the cache/cleanup version, resolved executable path, formatter version output, and style
file contents.
The formatter uses ``EnvironmentVariables::executableCandidatesOrThrow()`` for portable PATH lookup and native strings
for path arguments.
Cache records are streamed through ``StringSplitter``, and serialization reserves its final size once.
An explicit ``--full-format`` ignores prior fingerprints.
There is no timestamp-based periodic rescan.

All transformations must succeed before any source is written.
Configuration, resolution, parsing, or formatter failure leaves source files and the previous cache unchanged.
Output truncation is an error; truncated formatter output is never written.
The main thread reports results and replaces changed files through ``ProjectFiles``.
Equivalent files retain their original bytes and modification time.
Cache updates occur only after successful application; ``--check`` never writes them.
A filesystem failure during application can leave earlier successful replacements in place; this is not a multi-file
transaction.

Tests
=====

Focused tests cover include resolution, literal preservation, CMake semantics, spacing, cold and warm formatter runs,
style invalidation, failure-before-write behavior, and serial/parallel consistency.
Performance comparisons must use the optimized tool build and the same source corpus; measure cold full formatting
separately from warm pre-commit runs.
