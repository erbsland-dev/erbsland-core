..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************************
Time Test Generator Reference
*****************************

``tool::test_data::TimeTestsTool`` generates date, time, duration, and conversion fixtures.
See :doc:`../tool_usage/generate_time_tests` for the regeneration workflow.

Configuration and Options
=========================

The required ``[generate_time_tests]`` section has a text setting ``output_directory``, defaulting to
``test/unittest/data/time``.
Its checked-in configuration is ``tool/conf/tools/generate_time_tests.elcl``.
``--output-directory`` (``-o``) overrides the setting; ``--check`` (``-c``) compares without writing or creating
directories.
Paths must remain inside the project.
Configuration, path types, and all eleven destination paths are validated before generation.

Independent Calendar and Durations
==================================

``impl::CalendarDate``, ``impl::CalendarTime``, and ``impl::CalendarDateTime`` implement an independent proleptic
Gregorian model over years 0 through 9999. They do not call the Core date and time operations whose behavior the
fixtures test.
Calculations handle leap years, month-end clamping, range boundaries, and day transitions explicitly.
Extreme durations and conversions use exact integer arithmetic, including subtraction of the minimum signed 64-bit
value.

Local ``FastRandom(seed)`` instances produce repeatable samples without changing global random state.
The fixed seeds preserve reproducibility on a best-effort basis across Core versions.
Explicit boundary cases and coverage categories remain fixed.
Exhaustive calendar validation runs in a ``FullRun`` test rather than on every generator invocation.

Output and Rendering
====================

The eleven tables cover date and datetime epochs, date manipulation, date distances, time since midnight, datetime
duration addition, date/time/datetime comparisons, time-amount comparisons, and time-amount conversions.
Each file contains a schema header defining its field order.
The shared ``test_data/table.tpl`` template supplies notices, schema headers, and row iteration.
Native code supplies structured render values and formats exact large integers as decimal strings.

All tables are prepared before publication.
``ProjectFiles::updateText()`` retains equivalent existing files and atomically replaces each changed file.
An operating-system failure during publication can leave previously published files updated.
See :doc:`framework_reference` for comparison and path contracts and :doc:`rendering_reference` for notice
configuration.

Verification
============

``FixtureGeneratorsTest`` exercises explicit calendar boundaries, extreme durations, seeded repeatability, and command
validation.
Its exhaustive ``FullRun`` test validates every day in years 0 through 9999 and round trips the calendar's epoch
representation.
See :ref:`Run the tool tests <development-tool-setup-run-the-tool-tests>`.
