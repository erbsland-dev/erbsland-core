..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************************
Maintain Security Hashes
************************

``security-hashes`` records the infrastructure source files used by builds, maintenance commands, and documentation CI.
Run it after reviewing changes to those sources:

.. code-block:: console

    ./erbsland-core-tool security-hashes

Review the updated ``security_hashes.txt`` alongside the source changes.
The normal pre-commit workflow runs this command after the other maintenance commands.

Check Without Writing
=====================

.. code-block:: console

    ./erbsland-core-tool security-hashes --verify

``--check`` is an alias for ``--verify``.
Verification fails when the manifest is missing or outdated and never updates it.
Inspect the source changes before regenerating an outdated manifest.
A recorded digest detects changes relative to the reviewed manifest; it does not authenticate the manifest itself.

Control Parallel Work
=====================

The command hashes files concurrently using the logical CPU count, limited to 64 workers and the selected file count.
Use ``--jobs`` or ``-j`` to choose between 1 and 64 workers:

.. code-block:: console

    ./erbsland-core-tool security-hashes --verify --jobs 4

Worker count does not affect the manifest.
LF, CRLF, and CR line endings produce the same hashes, including when Git checks out files with Windows line endings.
Whitespace, byte order marks, and the presence of a final newline remain significant.

Select Infrastructure Sources
=============================

The ``[Security Hashes]`` section in ``tool/conf/tools/security_hashes.elcl`` selects source directories and individual
files.
The supplied configuration covers workflows, CMake code, documentation extensions, repository setup, Python tests, and
the C++ maintenance tool.
Configuration and generator templates are included; datasets, binaries, fixture data, and caches are excluded.
Ordinary library sources and demos remain outside this infrastructure selection.

See :doc:`../tool_reference/security_hashes_reference` for the exact selection and manifest contracts.
See :doc:`usage` for project selection, configuration overrides, and logging.
