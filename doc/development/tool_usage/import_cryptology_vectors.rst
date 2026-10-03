..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************************
Import Cryptology Vectors
*************************

``import-cryptology-vectors`` imports pinned upstream vectors into ``test/unittest/data/cryptology``.
The native command requires no Python interpreter or Python packages.

Import Online
=============

Run from the repository root.
The default mode downloads configured sources, verifies their SHA-256 pins, and prepares the fixtures in memory:

.. code-block:: console

    ./erbsland-core-tool import-cryptology-vectors
    ./erbsland-core-tool import-cryptology-vectors --family ed25519

Only sources for the selected family are downloaded.
The importer reuses an HTTP session, verifies HTTPS certificates, and bounds redirects, response sizes, and deadlines.
It creates no download directory or persistent cache.

Import Offline
==============

Place the required upstream files in a directory inside the project and explicitly select it:

.. code-block:: console

    ./erbsland-core-tool import-cryptology-vectors --family aes --source-directory staging/vectors

``--source-directory`` activates offline mode.
It has no default or configured directory.
Offline mode reads exclusively from that directory and never falls back to the network.
File names, upstream URLs, revisions, and attribution are listed in the generated cryptology ``README.rst``.
A complete import requires all twenty-two sources; a family import needs only that family's files.

Configuration and Overrides
===========================

Output settings, source URLs, checksum pins, and provenance are in ``tool/conf/tools/import_cryptology_vectors.elcl``.
Use ``--output-directory`` (``-o``) to override the configured ``output_directory`` for one invocation:

.. code-block:: console

    ./erbsland-core-tool import-cryptology-vectors --family sha --output-directory staging/cryptology

Source and output paths must remain inside the project.
Configuration and destination validation precede downloading or conversion; all outputs are prepared before publication.
Each changed file is replaced atomically.
The importer preserves unselected response files and unrelated files and builds its manifest from the selected output
directory's retained and proposed response files.

Check and Review Changes
========================

.. code-block:: console

    ./erbsland-core-tool import-cryptology-vectors --family sha --check

``--check`` (``-c``) reports differences without writing outputs or creating output directories.
It still uses the selected online or offline input mode.
Text checksum verification accepts both LF and CRLF files; no Git line-ending attributes are required.

Review the generated diff, run affected cryptology suites and native tool tests, and run the repository pre-commit
checks before accepting changes.
See :doc:`../tool_reference/cryptology_vector_importer_reference` for family choices, source validation, limits, and
manifest details.
