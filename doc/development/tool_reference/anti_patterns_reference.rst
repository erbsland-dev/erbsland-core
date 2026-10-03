..
    Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************
Anti-Patterns Reference
***********************

``tool::anti_patterns`` implements ``anti-patterns`` with various independent detectors.
The general ``unnecessary_conversion`` catalog entry has no detector.
Dedicated detectors cover :doc:`/guidelines/anti_patterns/unnecessary_unit_conversion`,
:doc:`/guidelines/anti_patterns/unnecessary_type_construction`, and :doc:`/guidelines/anti_patterns/namespace_noise`.
For commands and acceptance workflow, see
:doc:`../tool_usage/anti_patterns`.

Configuration
=============

The ``Anti Patterns`` section in ``tool/conf/tools/anti_patterns.elcl`` defines ``Source Directories``,
``File Suffixes``, optional rule sections, and zero or more ``Exception`` entries.
Shipped roots include library sources, demos, build helpers, tests, ``tool/src``, and ``tool/test/unittest/src``.
``Source Directories`` is a list of project-relative directory globs.
Each pattern matches complete directory paths; the matching directories are scanned recursively.
Plain directory paths remain valid, ``*`` matches within one path component, and ``**`` matches recursively.
Patterns are resolved anew for each full scan, in deterministic order, without following symlinks.
Overlapping selections scan each file once.
Empty patterns, malformed globs, absolute paths, parent traversal, and patterns matching no directories are rejected.
Explicit command-line targets remain literal files or directories and replace the configured selection for that run.

The shipped patterns select build helpers with ``cmake/tools/*/src`` and profiling programs with
``test/profiling/*/src`` plus ``test/profiling/*/test/src`` for nested test suites.
This excludes sibling data and resource templates while preserving source domains such as ``test/unittest/src/data`` and
``test/unittest/src/resource``.
Fuzz targets and their shared fixture are selected with ``test/fuzzing/*/src``; checked-in corpora remain outside the
scan.

Each exception requires nonempty string lists for ``Path`` and ``Rules``, and a nonempty ``Reason``.
A string list can consist of a single value.
Each path is compiled with Core's ``PathGlobPattern`` and matches the complete project-relative path.
The same rules and reason apply when any listed path matches.
Directory coverage is explicit, for example ``demos/**``.
Unknown rules, blank paths, duplicate path/rule pairs within or across entries, invalid globs, and unsafe paths are
rejected.

Rule Settings
-------------

Every registered detector has an optional subsection named after its rule identifier.
For example, ``Namespace Noise`` and ``namespace_noise`` name the same ELCL section.
Rules own their setting schemas and replace their settings before scan workers start.
Unknown sections or settings and invalid values are rejected.
All settings below accept one text value or a text list; omitted settings are empty lists.
Names and exemption texts are case-sensitive, and blank entries are invalid.
Project-specific lists live in the shipped ELCL file rather than detector defaults.

.. code-block:: erbsland-conf

    --[ Anti Patterns . Namespace Noise ]--
    Domains         : "text", "unit", "err", "mem", "options", "path", "stream"
    Ignored Symbols : "mem::secureErase"

``Namespace Noise``
    ``Domains`` contains C++ identifiers whose qualifications are counted independently.
    ``Ignored Symbols`` contains complete domain-qualified identifiers.
    Ignored occurrences do not count toward the threshold and cannot determine the reported location.
    Matching recognizes the same symbol through ``el::`` and ``erbsland::`` qualifications, including token spacing
    and comments; similarly named symbols remain eligible.

``Named Constant For Repeated Literals``
    ``Key Functions`` contains exact function names and ``Key Function Prefixes`` contains function-name prefixes.
    Their first literal argument is treated as a key.
    The shipped configuration retains the existing option, validation-rule, and value-access function names.
    Demo acceptance is configured through the ``demos/**`` exception.

``Missing Api Documentation``
    ``File Exemption Texts`` contains class or struct header substrings that exempt the whole file.
    ``Declaration Exemption Texts`` contains declaration substrings that exempt individual API declarations.
    The shipped values are ``UnitTest`` and ``PrivateTag``, respectively.
    Comments and string literals do not activate these exemptions.

Scanning and Cache
==================

One inventory selects supported files.
Each changed source is read once and tokenized once into shared line, comment, literal, directive, scope, and
declaration indexes.
Every detector examines that index and returns source spans.
Files are independent tasks in a Core ``WorkerPool``; results are merged into stable report order.
Findings inside ``ERBSLAND_OS`` conditional branches are excluded.
Adjacent inline acceptance comments are recorded with each source finding.
Configured exceptions are applied after cache lookup.

The versioned JSON cache at ``.cache/anti_patterns.json`` records per-file metadata, the running binary identity, the
rule configuration identity, and source findings with inline reasons.
A missing, incompatible, or malformed cache causes a rescan.
Changing rule settings invalidates cached detection results.
Changing source-directory patterns rebuilds the inventory but reuses unchanged selected files; a full scan removes cache
entries outside its inventory.
Changing configured path exceptions does not invalidate detection results.
Cache writes are atomic through the framework's project file service.
The cache is disposable and cannot make a scan fail.

Reports
=======

Findings are ordered by severity, rule, path, and line.
Missing API documentation and default-group comment findings are grouped per file and acceptance, with the first
location shown.
Report rows contain rule identifiers and links to the matching guideline pages.
The summary counts all active and accepted findings, including rows beyond ``--limit``.
