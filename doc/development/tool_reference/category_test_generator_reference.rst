..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*********************************
Category Test Generator Reference
*********************************

``tool::test_data::CategoryTestDataTool`` generates the fixture consumed by ``CategoryContainsTest``.
See :doc:`../tool_usage/generate_category_test_data` for commands and configuration.

Independent Expectations
========================

The generator reads validated ``UnicodeData.txt`` records through ``UnicodeSource``.
First/Last records form inclusive ranges.
Missing code points have general category ``Cn``.
The covered domain is U+0000 through U+10FFFF, including surrogate code points.
``impl::CategoryTestData`` derives expression names directly from the source general category and explicit ASCII,
whitespace, word, and dot rules.
It does not call ``Category::contains()`` or use generated runtime Unicode tables.

The expressions preserve the original imported fixture's mask representation.
Some category enumerators share bits; the presence or absence of an individual name is not a list of all matching
categories.
``Any`` is omitted only for LF; the fixture consumer reconstructs the mask and tests every category against it.
Keep these independent expectations aligned with the documented regex behavior when intentionally changing that
behavior.

Sampling and Output
===================

Adjacent code points with identical expressions form a single range, including across source-record boundaries.
Each range emits its first code point, its last if different, and its integer midpoint when it contains at least three
code points, in that order.
Code points use six uppercase hexadecimal digits; deduplicated expression names are sorted alphabetically and separated
by a vertical bar with one space on each side.
A semicolon separates the code point from its expression.

The native port reproduces all 7,698 data rows in the imported Unicode 17.0.0 fixture.
Its provenance header identifies the pinned Unicode version and regeneration command.
``--check`` performs a read-only comparison using the shared fixture publication contract.

Verification
============

``CategoryTestDataTest`` checks explicit category expectations, whitespace boundaries, adjacent-range merging, sampling,
unassigned gaps, and the upper Unicode boundary.
Its ``FullRun`` tests compare the complete pinned fixture and exercise source changes, check mode, preserved line
endings, and refusal to publish after source-integrity failures.
