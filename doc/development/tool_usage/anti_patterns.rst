..
    Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************
Check Anti-Patterns
*******************

``anti-patterns`` checks the documented C++ patterns in the library, demos, build helpers, maintenance tool, and tests.
It is run by ``pre-commit`` after code cleanup.

Run the Command
===============

From the project root, run a full scan or select project-relative files and directories:

.. code-block:: console

    ./erbsland-core-tool anti-patterns
    ./erbsland-core-tool anti-patterns src/erbsland/text tool/src/tool/anti_patterns
    ./erbsland-core-tool anti-patterns --jobs 4 --limit 50
    ./erbsland-core-tool anti-patterns --show-suppressed

``--jobs`` accepts 1 through 64 workers; the default is the bounded logical CPU count.
``--limit`` accepts 1 through 200 report rows and defaults to 20. Active findings make the command fail.
Accepted findings appear in the summary and can be inspected with ``--show-suppressed``.

Configure Source Selection and Rules
====================================

``Source Directories`` in ``tool/conf/tools/anti_patterns.elcl`` accepts project-relative directory globs.
For example, ``cmake/tools/*/src`` and ``test/profiling/*/src`` select source trees without scanning sibling data or
resource templates.
Each matched directory is scanned recursively; each pattern must match at least one directory.
``**`` can select directories across multiple levels.
Explicit command-line paths remain literal targets and replace this selection for that run.

Rules can expose optional settings beneath their own section:

.. code-block:: erbsland-conf

    --[ Anti Patterns . Namespace Noise ]--
    Domains         : "text", "unit", "err", "mem", "options", "path", "stream"
    Ignored Symbols : "mem::secureErase"

Here, the ignored symbol does not contribute to namespace-noise counts.
Omitted rule settings are empty lists; the shipped configuration supplies this project's policy.
See the reference for the available settings.

Accept a Finding
================

Fix the code when possible.
For intentional code, place an adjacent ``// anti-pattern: allow <rule> -- <reason>`` comment before the construct or
after its statement.
To accept a file or directory location, add a reasoned ``Exception`` block to ``tool/conf/tools/anti_patterns.elcl``.
Use an explicit recursive glob for directory coverage, such as ``tool/test/unittest/src/**``.
List several rule identifiers together only when the same reason explains each one.
``Path`` also accepts a list when the same rules and reason apply to several locations:

.. code-block:: erbsland-conf

    --*[ Anti Patterns . Exception ]*--
    Path   : "src/First.cpp", "src/Second.cpp"
    Rules  : "regular_string_literal"
    Reason : "These adapters pass narrow C strings to native interfaces."

Multi-line lists with one ``*`` entry per path are also supported.

The scanner caches source findings in ``.cache/anti_patterns.json``.
Changes to a source file, the running tool binary, or rule settings invalidate the relevant results.
Source-pattern changes refresh the selected inventory and retain reusable results for unchanged selected files.
Configured exceptions are applied after cache lookup, so editing an exception does not require a rescan.
A missing or damaged cache is rebuilt automatically.

See :doc:`../tool_reference/anti_patterns_reference` for validation, rule registration, and scanning details.
The
:doc:`../../guidelines/anti_patterns/index` catalog explains each rule.
