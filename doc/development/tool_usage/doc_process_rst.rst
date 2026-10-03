..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************************
Process RST Documentation
*************************

Normalize headings, simple paragraphs, and C++ links in RST files.
A normal invocation performs cleanup only; demo execution requires an explicit ``--demos`` selection.

.. code-block:: console

    ./erbsland-core-tool doc-process-rst doc/development --recursive

Use ``--dry-run`` to preview changes and ``--check`` to fail if normalization is required, without writing documents.
Cleanup requires the Doxygen XML index, refreshed by ``doc-build`` after source API changes.

Demo Processing
===============

``--demos sync`` updates stale excerpts and captures.
``--demos validate`` checks synchronization without publishing generated demo changes.
``--demos refresh`` recursively regenerates all blocks, and ``--demos migrate`` converts legacy metadata only.
``--no-cleanup`` selects demo-only operation, and ``--force`` regenerates blocks despite matching hashes.
Explicit demo processing may execute demos during preview and check modes.

Demo updates happen before cleanup, with one final document write.
Build selected demo executables first; capture works on macOS, Linux, and Windows and is bounded by time and output
limits.
The ``Demo Executable Root`` configuration setting defaults to ``cmake-build-debug/demo-apps`` on every platform.
Set a project-relative or absolute directory for a special build.
Review modified source excerpts, expected failures, fixture use, and terminal output.

Source Presentation
===================

Use the ``Demo Text Replacement`` section list to simplify generated source excerpts without changing compiled demos.
Each entry has a regular-expression ``Pattern`` and a ``Replacement`` using Core capture references.
Rules run in declaration order, after ``:show:`` and ``:hide:`` select the source.
For example, this configuration presents the demo capture helper as the public application API:

.. code-block:: text

    *[ Doc Process Rst . Demo Text Replacement ]
    Pattern: /(?:demo::)?DemoTerminal::enable\((?<application>[A-Za-z_]\w*)\);/
    Replacement: "{application}.enableTerminal();"

The repository configuration also handles ``DemoTerminal::enable(*this)``.
Replacement rules are excluded from demo fingerprints.
After changing a rule, manually review the affected excerpts and use ``--demos sync --force`` on those pages to refresh
them.

See :doc:`usage` for shared invocation options and
:doc:`../tool_reference/documentation_tools_reference` for the technical contracts.
