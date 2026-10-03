..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******************************
Include Dependencies Reference
******************************

See :doc:`../tool_usage/include_dependencies` for examples and :doc:`framework_reference` for shared tool behavior.

Command Interface
=================

.. list-table::
    :header-rows: 1

    * - Mode
      - Arguments and options
    * - ``summary``
      - Default mode; no arguments.
    * - ``tree HEADER``
      - One source identifier; ``--reverse`` follows dependents and ``--max-depth N`` limits depth to a nonnegative value.
    * - ``path SOURCE TARGET``
      - Two source identifiers; prints a deterministic shortest internal chain or fails if none exists.
    * - ``cycles``
      - No arguments; lists strongly connected components containing cycles, including self-includes.
    * - ``export``
      - ``--format json|dot`` defaults to JSON. Optional ``--output NAME`` selects a confined report filename.
    * - ``check [RULE]``
      - Checks all configured rules or one case-sensitive name. ``--all`` also lists every affected public header.

Irrelevant options and incorrect argument counts fail before scanning.
Source identifiers accept source-relative paths, paths prefixed by ``src/erbsland/``, or unique path-component suffixes.
Ambiguous and unknown identifiers fail with option context.
Tree output retains directive line numbers and marks cycles, previously visited nodes, and depth limits.

Configuration
=============

The required ``[Include Dependencies]`` section contains these settings:

.. list-table::
    :header-rows: 1

    * - Setting
      - Contract
    * - ``Public Header Map``
      - Text; defaults to ``tool/data/public_headers/header_map.txt``. Must identify a valid mapping file.
    * - ``Git Executable``
      - Text; defaults to ``git``. A trusted executable is required for named exports.
    * - ``Safe Binary Directories``
      - Required nonempty text list of absolute trusted directory roots.

Executable trust follows :doc:`github_workflows_reference`; Git is launched only for named exports.

At least one ``Required Dependencies`` section-list entry is required.
Each entry supplies a unique ``Name`` containing ASCII letters, digits, underscores, or hyphens; a source-relative
``Target``; and optional ``Excluded Headers`` text list.
Targets and exclusions must identify inventoried files and cannot contain dot components or backslashes.
The checked-in ``definitions`` rule requires ``core/Definitions.hpp`` and exempts ``core/Namespaces.hpp``.
Configuration errors retain the offending value's location.

Graph and Reports
=================

The inventory contains regular ``.hpp``, ``.tpp``, and ``.cpp`` files under ``src/erbsland``; symbolic links are
skipped.
Core's C++ tokenizer identifies literal include directives outside comments and literals, including continued directive
lines.
Conditional branches are all included.
Macro-generated include names are ignored; this graph does not run a preprocessor.
Quoted includes must resolve to inventoried sources.
Angle-bracket includes remain external nodes.

Public membership comes from the canonical-first publication map shared with code cleanup.
Forward headers and ``all.hpp`` or ``all_*.hpp`` umbrellas receive their respective roles and are excluded from regular
public-header checks.
The map rejects malformed paths, unsorted source records, duplicate pairs, and empty contents while retaining aliases
and merged public wrappers.

Forward and reverse adjacency are indexed once.
Shortest paths use breadth-first traversal.
Iterative strongly connected component analysis supports cycle reports and dependency checks.
Checks use reverse reachability from each target, then propagate missing-public-header membership over the component
graph to select one public insertion point per terminal affected component, including paths through private nodes.

JSON retains schema version 1: ``schema``, ``summary``, ``nodes``, ``edges``, and ``cycles``.
Internal nodes include path, role, public membership, and direct/transitive dependency/dependent counts.
External node paths are wrapped in angle brackets.
Edges contain source, target, one-based physical ``line``, ``internal``, and ``system``.
DOT retains nodes, public-header box styling, external-node dashed styling, and include-line edge labels.
Exports have deterministic ordering and stdout contains only the requested serialization.

Named reports are written under ``temporary_data/include_dependencies``.
``NAME`` must be a filename; an omitted suffix is added and a supplied suffix must match the selected format.
Parent links and escapes are rejected.
Before scanning or creating directories, Git must confirm that the path is untracked and ignored.
Changed reports use atomic replacement.
Source files are never modified by this command.

Verification
============

Native fixtures cover parsing, graph traversal, public scope, cyclic insertion frontiers, JSON/DOT output, diagnostics,
and export safeguards.
Compiled fake Git responses exercise tracking and ignore decisions without network access.
