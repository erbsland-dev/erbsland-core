..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************
Build the Documentation
***********************

Build the documentation incrementally from the repository root:

.. code-block:: console

    ./erbsland-core-tool doc-build

On Windows use ``tool\bin\erbsland-core-tool.exe doc-build``.
The command uses the Sphinx executable in the selected project's ``.venv`` without requiring you to activate it.
Run ``setup.py`` again after updating the checkout to refresh the installed maintenance tool.
For a command under development, build and run ``cmake-build-debug/tool/erbsland-core-tool`` instead.

Incremental and Full Builds
===========================

Output goes to ``_build``.
Local builds use a ``development`` version label.
The GitHub documentation workflow fetches the complete Git history and derives the published version from the nearest
reachable release tag, using the same tag naming convention as the library.
Pushing a release tag triggers publication as well as pushes to ``main``.
Incremental runs reuse Sphinx doctrees and Doxygen XML.
The Sphinx extension refreshes API input and XML when source headers or preprocessing settings change.
Direct Sphinx and Sphinx-autobuild invocations continue to share these caches; see :doc:`../setup`.

Discard all generated documentation caches when output becomes stale or before publishing a release:

.. code-block:: console

    ./erbsland-core-tool doc-build --full-rebuild

This removes ``_doxygen_input`` and ``_build`` before launching Sphinx.
``--force`` is another spelling for ``--full-rebuild``.

Diagnostics
===========

By default, the command shows Sphinx warnings and errors and Doxygen warnings that have no configured exception.
It shortens generated paths, summarizes accepted warnings, reports the build result, and displays elapsed time.
If Sphinx fails, it replays all delivered output, including details without a diagnostic prefix.

Use ``--no-filter`` to inherit complete Sphinx/Doxygen output, ``--show-suppressed`` to display accepted warnings while
retaining concise output, and ``--verbose`` to include command and cache-removal details.

Warning Exceptions
==================

Warning exceptions live in ``tool/conf/tools/doc_build.elcl``.
When a warning is intentional, add a narrow source-specific exception and explain why it is acceptable.
Use ``--show-suppressed`` to review accepted warnings.
The :doc:`reference <../tool_reference/doc_build_reference>` lists every exception field and its matching rules.

See :doc:`usage` for project selection, alternate configuration files, and logging.
