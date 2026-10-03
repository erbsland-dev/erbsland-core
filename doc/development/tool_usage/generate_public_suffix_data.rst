..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********************************
Generate Public Suffix List Data
********************************

``generate-public-suffix-data`` generates the HTTP cookie implementation's compact Public Suffix List tables.
It reads the complete pinned ICANN and PRIVATE snapshot in ``tool/data/public_suffix`` and the shared configured Unicode
sources in ``tool/data/ucd``.
Generation is offline and uses no Python packages.

Check or Regenerate the Tables
==============================

Run these commands from the repository root:

.. code-block:: console

    ./erbsland-core-tool generate-public-suffix-data --check
    ./erbsland-core-tool generate-public-suffix-data

``--check`` (``-c``) fails if the output is missing or meaningfully different, without writing.
Normal generation validates the data and updates changed output, leaving equivalent files untouched.

Configuration and Overrides
===========================

Output settings are in ``tool/conf/tools/generate_public_suffix_data.elcl``.
The default destination is ``src/erbsland/network/impl/http/cookie/PublicSuffixData.cpp``.
Use ``--output-file`` for a one-off output in an existing project directory:

.. code-block:: console

    ./erbsland-core-tool generate-public-suffix-data --output-file tool/data/PublicSuffixData.cpp

Select the list snapshot in ``tool/conf/source/public_suffix.elcl`` and the shared Unicode snapshot in
``tool/conf/source/unicode.elcl``.
There are no command-line input overrides.
A custom snapshot needs its matching manifest; see :doc:`../tool_reference/public_suffix_generator_reference` for
configuration and the manifest format.
See :doc:`rendering_configuration` to customize generated notices.

Update the Snapshot
===================

Follow :doc:`update_public_suffix` to acquire and verify a new snapshot before regenerating the tables.
For rule preparation, encoding, and verification, see :doc:`../tool_reference/public_suffix_generator_reference`.
