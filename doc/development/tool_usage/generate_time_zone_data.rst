..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************
Generate Time-Zone Data
***********************

``generate-time-zone-data`` builds the compact IANA time-zone database and Windows mapping from the checked-in tzdb
2026b and CLDR 48 snapshots under ``tool/data/time_zone``.
Run it after changing those snapshots or the generator.

Check or Regenerate
===================

Run from the repository root with the tool built from the current checkout:

.. code-block:: console

    ./erbsland-core-tool generate-time-zone-data --check
    ./erbsland-core-tool generate-time-zone-data

The command writes the region factories, text and rule tables, zone dispatch, database version, and Windows mapping in
``src/erbsland/time/tz/impl``.
``--check`` (``-c``) reports missing or stale files without writing.
Review zone IDs, transitions, abbreviations, and CLDR mappings, then run the affected time tests.

Configuration
=============

``tool/conf/tools/generate_time_zone_data.elcl`` sets ``output_directory``.
Use ``--output-directory`` (``-o``) for a one-off comparison in an existing project directory.
``tool/conf/source/time_zone.elcl`` selects the source directory.
The source manifest pins both releases and exact file hashes.
See
:doc:`../tool_reference/time_zone_generator_reference` for the input and representation contracts; see :doc:`usage`
for shared tool options.
