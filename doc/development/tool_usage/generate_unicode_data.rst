..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*********************
Generate Unicode Data
*********************

``generate-unicode-data`` builds the Unicode Light, version, and normalization tables from the verified UCD snapshot in
``tool/data/ucd``.
Run it after changing the generator or following :doc:`update_unicode`.

Check or Regenerate
===================

Run from the repository root with the tool built from the current checkout:

.. code-block:: console

    ./erbsland-core-tool generate-unicode-data --check
    ./erbsland-core-tool generate-unicode-data

The command writes ``UnicodeData.cpp``, ``UnicodeVersion.cpp``, and ``UnicodeNormalizationData.cpp`` to
``src/erbsland/text/impl``.
``--check`` (``-c``) reports missing or stale files without writing.
Generation leaves equivalent files untouched.
Review changes to the encoded tables and run the affected text tests.

Configuration
=============

``tool/conf/tools/generate_unicode_data.elcl`` sets ``output_directory``.
Use ``--output-directory`` (``-o``) for a one-off comparison in an existing project directory.
``tool/conf/source/unicode.elcl`` selects the input snapshot and version pin.
The command has no input override.
See :doc:`../tool_reference/unicode_data_generator_reference` for source validation and table limits; see :doc:`usage`
for shared tool options.
