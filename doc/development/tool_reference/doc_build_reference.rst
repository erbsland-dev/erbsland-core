..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************************
Documentation Build Reference
*****************************

``tool::doc::BuildDocTool`` launches the selected checkout's virtual-environment Sphinx executable directly through
Core's subprocess API.
See :doc:`../tool_usage/doc_build` for the build workflow.

Options and Configuration
=========================

The command requires ``[doc_build]`` in ``tool/conf/tools/doc_build.elcl``.
An empty section accepts no Doxygen warning exceptions.

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Option
      - Behavior
    * - ``--full-rebuild``, ``--force``
      - Remove ``_doxygen_input`` and ``_build`` before execution; disabled by default
    * - ``--no-filter``
      - Inherit complete subprocess output instead of filtering; disabled by default
    * - ``--show-suppressed``
      - Display accepted warnings in filtered mode; disabled by default, irrelevant with ``--no-filter``

``ignored_doxygen_warnings`` is an optional section list.
Each entry has these text fields:

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Field
      - Default and meaning
    * - ``source_glob``
      - Required and nonempty; matches the complete shortened source path, case-sensitively
    * - ``message_contains``
      - Empty by default; optional message substring condition
    * - ``message_regex``
      - Empty by default; optional Core regular-expression condition
    * - ``reason``
      - Required and nonempty; explanation reported with accepted warnings

For example:

.. code-block:: text

    [Build Doc]

    *[Build Doc.Ignored Doxygen Warnings]
    Source Glob: "erbsland/example/impl/Helper.hpp"
    Message Contains: "The following parameter"
    Reason: "The constructor is documented through its public interface."

Both supplied message conditions must match.
Source globs use :doc:`/reference/path/path_glob_patterns` with full matching.
``*`` and ``?`` stay within a component; ``**`` spans zero or more components.
Use Core bracket classes, including ``[^a-z]`` for negation.
Migrate former separator-spanning ``*`` patterns to whole-component ``**`` where needed.
Entries are evaluated in configuration order.

Execution
=========

Sphinx runs with arguments ``-j 1 doc _build`` and the selected project root as its working directory.
The child receives a restricted environment with locale, home, temporary-directory and PATH settings, plus
``VIRTUAL_ENV`` and the virtual-environment executable directory prepended to PATH.
Windows also retains ``SystemRoot`` and ``WINDIR``.
The launcher must be a confined executable regular file no larger than 1 MiB.

The default build leaves generated directories intact.
A full build removes only the two fixed cache directories through Core path operations, retrying removal up to three
times.
Symlinks, reparse points, and non-directory cache paths are rejected.
The Sphinx extension remains responsible for preparing generated API inputs and refreshing the Doxygen cache.

Filtering
=========

Filtered mode uses a local event loop and the ``System`` frontend, with stderr merged into stdout.
Raw output blocks are assembled into lines; a final unterminated line is also processed.
All delivered lines are retained for replay if Sphinx fails.
The pending-output buffer is bounded to 64 MiB.
If the consumer falls behind and drops output, a warning identifies incomplete failure replay; Sphinx's exit result is
unchanged.
Unfiltered mode inherits the native output streams and uses procedural waiting.

Uppercase Sphinx warning/error markers and lowercase error markers remain visible.
Doxygen warning recognition is case-insensitive.
Only explicitly configured source/message matches are suppressed, with counts reported in first-suppressed order.
Generated Doxygen and Sphinx source prefixes are shortened before display.
``StringPattern::fromGlob()`` provides the fnmatch-style source matching required by the migrated configuration.

Verification
============

``DocumentationOutputFilterTest`` covers diagnostic visibility, path normalization, rule matching, counts and globs.
``BuildDocToolTest`` invokes a small native Sphinx fixture in an isolated project to check arguments, environment, cache
handling, safety checks, output filtering, full failure replay, and exit diagnostics.
Core subprocess suites separately cover native lifecycle, chunking, event delivery and overflow.
