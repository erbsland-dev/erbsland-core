..
    Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************
Clean Up the Code
*****************

``code-cleanup`` repairs and sorts C++ includes, fills the project's source-layout gaps in clang-format, normalizes
simple CMake source lists, and runs clang-format only for files whose code needs formatting.

Run the Command
===============

Refresh public-header mappings before cleanup when headers have moved or publication policies have changed:

.. code-block:: console

    ./erbsland-core-tool update-public-headers
    ./erbsland-core-tool code-cleanup

The native ``pre-commit`` command runs these operations in that order before its remaining checks.
It still reports an error when maintenance changes tracked or untracked project content, so you can review changes and
rerun pre-commit.

The first cleanup run formats every configured C++ source to establish a verified baseline.
This can take longer than later pre-commit runs.
The tool processes files concurrently; use ``--jobs 4`` to limit it to four workers and at most four formatter
processes.

Later runs still repair include paths, sort include blocks, check CMake lists, and enforce project spacing.
They skip clang-format when the code outside the leading include region matches the cached baseline.
Changing an include path or adding an include therefore does not force an expensive formatter run.

.. code-block:: console

    ./erbsland-core-tool code-cleanup --check
    ./erbsland-core-tool code-cleanup --jobs 4
    ./erbsland-core-tool code-cleanup --full-format

``--check`` reports required changes and exits unsuccessfully without writing sources or the cache.
``--full-format`` explicitly refreshes the full formatting baseline.
There is no periodic full-format run.
Changes to the style file or formatter identity invalidate the baseline automatically.

Configure Source Trees
======================

Edit ``tool/conf/tools/code_cleanup.elcl`` to select source trees and local include roots.
Each named source policy has a ``Directories`` list and an ``Include Style``.
Directory entries are project-relative globs, such as ``test/fuzzing/*/src``; each matched directory is scanned
recursively.

* ``relative`` is reserved for the library implementation directory.
  Core includes become paths relative to the source file.
* ``public`` selects canonical includes under ``Public Include Prefix`` (``<erbsland/...>`` by default) for consumers.
  Existing local headers and generated ``.in`` templates retain their spelling.

``Include Roots`` are additional compiler search roots for that tree.
They recognize local headers without adding those directories to the cleanup scope.
Duplicate or nested selections within one policy are collapsed.
Different policies must not overlap, so each C++ file has exactly one policy.
Every pattern must match at least one directory; unsafe paths, malformed patterns, and links are rejected or skipped.

CMake configuration maps directories to explicit target names.
The most specific mapping wins; this distinguishes the tool executable, tool library, and tool unit tests.
Unsupported CMake syntax is preserved and listed with ``--verbose``.

See :doc:`../tool_reference/code_cleanup_reference` for the full configuration, transformation boundaries, cache
invalidation, and failure behavior.
