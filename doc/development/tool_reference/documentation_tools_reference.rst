..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************************
Documentation Maintenance Reference
***********************************

These commands share the framework's project discovery, configuration selection, logging, and error reporting.
Project paths are confined by ``ProjectFiles``; explicit symlinks and paths leaving the project are rejected.
All commands return success when their operation completes and failure for invalid inputs or failed checks.

RST Processing
==============

``doc-process-rst`` requires one RST file or directory argument.
``--recursive`` includes subdirectories; ``--dry-run`` reports proposed changes; ``--check`` additionally fails when
changes are needed.
Neither option writes documents.
``--no-cleanup`` requires explicit demo processing.

``--demos`` accepts ``validate``, ``sync``, ``refresh``, or ``migrate`` and is absent by default.
Refresh implies recursive traversal and forced regeneration; ``--force`` independently bypasses matching demo hashes.
Validation reports stale blocks, and synchronization errors produce contextual diagnostics and generated notes.
Generated demo content and code blocks are protected from ordinary cleanup.

The ``doc_process_rst`` configuration section has ``Line Width`` (integer, minimum 20, default 120) and
``Doxygen Index`` (project-relative XML file, default ``_build/breathe/doxygen/erbsland-core/xml/index.xml`` .
``Demo Executable Root`` selects a project-relative or absolute directory, defaulting to ``cmake-build-debug/demo-apps``
on every platform.
The XML index is read only when cleanup is enabled.
The optional ``Demo Text Replacement`` section list contains a regular-expression ``Pattern`` and text ``Replacement``
in each entry.
Rules replace all matches in declaration order after source selection, before embedding the excerpt.
Replacement expressions use Core capture references such as ``{name}`` or ``{1}``; double braces represent literal
braces.
The rules affect generated C++ excerpts only, leaving source files, handwritten RST, and captured output unchanged.
Replacement rules are excluded from demo fingerprints; changing or removing a rule requires manual review and a forced
synchronization of affected excerpts.
Invalid patterns and replacement references fail before any documents are written.
Unresolved or ambiguous C++ references become literals with warnings.
Simple paragraphs preserve inline markup and break at sentence boundaries, recognizing common abbreviations.

Demo Contracts
==============

The directive format and selectors are specified in :doc:`/guidelines/demo_integration`.
Executables are selected below ``Demo Executable Root``; native executable suffixes are applied without searching PATH.
Arguments are parsed without invoking a shell.
Author-provided fixtures must be confined existing files with unique basenames.
Built-in file/directory placeholders and direct demo fixtures are isolated in temporary storage.
Source and input files are limited to 1 MiB; capture is limited to 10 seconds, 1 MiB, and 100 physical output lines.

Capture uses ``Subprocess`` with merged stdout/stderr, closed stdin, and restricted environment inheritance.
It enables the demo virtual terminal through ``ERBSLAND_VIRTUAL_TTY`` and supplies dimensions through
``ERBSLAND_VIRTUAL_TTY_SIZE``.
Capture works on macOS, Linux, and Windows.
Timeout cleanup terminates the direct child; captured commands must manage their own child processes.
The capture is redacted before normalization through ``TerminalEmulation`` at 90×40, with 2,000 retained scrollback
rows.
Regular captures combine populated history with the fixed frame; alternate captures use only the alternate frame.
Serialized output uses ANSI with both trimming policies enabled.
The migration removes the ``erbsland-ansi-convert`` runtime dependency.

Reference Generation
====================

``doc-update-reference`` reads ``doc_update_reference`` configuration.
``Source Directory`` defaults to ``src/erbsland`` and ``Reference Directory`` to ``doc/reference``.
``Excluded Directory Names``, ``Excluded Header Names``, and ``Excluded Header Globs`` are optional text lists.
``Exclude Underscore Headers`` defaults to true.
``Manual Reference Pages`` is an optional list of reference-relative non-index RST paths preserved during cleanup.

``Reference Groups`` is a section list.
Each entry requires a unique reference-relative ``Page`` ending in ``.rst`` and at least one selected header.
``Title`` is optional; ``Headers``, ``Header Globs``, and ``Excluded Header Globs`` are optional source-relative lists.
Selected headers must exist, pass exclusions, and belong to exactly one group.

Only documented namespace-scope classes, structs, enums, aliases, concepts, and supported free functions become Breathe
entries.
Authoritative matching forward declarations are included and duplicate entries are emitted once.
Existing pages require one final Interface section; indexes require one toctree.
Unowned non-index pages are removed unless registered as manual pages.

Guideline and Coverage Validation
=================================

``doc-api-guidelines`` reads domain pages in ``doc/guidelines/api``, excluding ``common.rst``.
It checks structural sections, type entries, pattern definitions, call patterns, and return types.
``doc-api-test-status`` validates library header markers; ``--normalize`` applies legacy syntax and placement cleanup.
Neither command has an individual configuration section.

Doxygen Preprocessing and Builds
================================

``doc-preprocess-sources`` has no individual configuration section and accepts ``--force``.
It excludes aggregate headers, transforms trailing return syntax, strips selected macro/helper constructs, and preserves
constraints needed to distinguish overloads.
The generated tree is ``_doxygen_input``; a versioned marker fingerprints source paths, content, and modification times.
Legacy markers are invalidated once.
Unchanged input preserves the output tree and downstream XML cache.

Sphinx calls this command before its existing Doxygen configuration/cache logic.
It uses the installed repository executable unless ``ERBSLAND_CORE_TOOL`` selects an explicit executable.
``doc-build`` supplies its own executable through that variable.
See :doc:`doc_build_reference` for build options and diagnostic filtering.

Demo Creation
=============

``create-demo`` requires ``<domain>/<DemoName>/<DemoPart>`` and optionally ``--yes``.
Domains use lowercase letters and underscores; collection/part names start uppercase and continue with ASCII letters or
digits.
New snippet collections go into ``demos/<domain>/snippets/<DemoName>``.
Existing legacy collections at ``demos/<domain>/<DemoName>`` remain extendable in place.
Existing collections must use the supported multi-part schema.
If both locations exist, or the same name exists under ``apps`` or ``templates``, creation fails without writing files.
Category and domain CMake registration and the printed source path match the selected location.
The command validates and displays its complete plan before interactive confirmation, then applies confined atomic file
updates.
A project-scoped interprocess lock serializes concurrent creation.
Generator rendering templates are compiled resources; related CMake entries, declarations, and registrations are sorted.
Missing matching library domains or headers produce naming warnings.

Usage Guides
============

* :doc:`../tool_usage/doc_process_rst`
* :doc:`../tool_usage/doc_update_reference`
* :doc:`../tool_usage/doc_api_guidelines`
* :doc:`../tool_usage/doc_api_test_status`
* :doc:`../tool_usage/doc_preprocess_sources`
* :doc:`../tool_usage/create_demo`
