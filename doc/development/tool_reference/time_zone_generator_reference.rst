..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************************
Time-Zone Generator Reference
*****************************

``tool::time_zone::TimeZoneDataTool`` builds the compact time-zone database and Windows mapping.
See :doc:`../tool_usage/generate_time_zone_data` for the workflow.

Configuration and Source Contract
=================================

The required ``[generate_time_zone_data]`` section has text setting ``output_directory``, defaulting to
``src/erbsland/time/tz/impl``.
``--output-directory`` (``-o``) overrides it for one run.
``--check`` (``-c``) compares all 25 outputs without writing and fails if any is missing or differs.
The output directory must exist inside the project.

The ``[time_zone]`` section has text setting ``input_directory``, defaulting to ``tool/data/time_zone``.
The directory contains tzdb ``version`` and nine region files, CLDR ``windowsZones.xml``, and ``manifest.elcl``.
The manifest pins tzdb 2026b and CLDR 48 and supplies SHA3-256 hashes of normalized input text.
Generation is offline; the copied source files alone supply zone histories, rules, aliases, abbreviations, and Windows
mapping facts.
Core's XML parser and Gregorian calendar arithmetic do not consult its generated time-zone tables.

Parsing and Encoding
====================

The model reads region files in the specified order, preserving zone continuation and rule order.
Primary zone names sort lexicographically and receive IDs from one; named rule sets sort lexicographically and receive
indexes from zero.
Aliases resolve to primary names.
Path components and possible abbreviations form a sorted text table; each fragment has at most 15 bytes, and the packed
text block uses 16-bit offsets.

Rules encode year span, month, day expression, transition reference and time, and saving offset in 64-bit words.
Continuation lines encode standard offset, UTC ``UNTIL`` boundary, effective offset, rule selection, and abbreviation
index.
UTC, wall, and standard time references follow the tzdb suffixes.
The renderer retains the runtime table layouts and IDs.
CLDR mappings use entries with territory ``001`` and the first IANA name in ``type``; aliases are resolved before
encoding the Windows key's zone ID.

The source reader, tzdb model and calendar calculations, table preparation, and render templates are separate units.
``TimeZoneDataTool`` renders every output before updating any destination.

Verification
============

``TimeZoneDataToolTest`` checks source hashes and version pins, changed and malformed records, representative
transitions, CLDR mapping, and output limits.
The affected Core time-zone tests verify runtime behavior.
