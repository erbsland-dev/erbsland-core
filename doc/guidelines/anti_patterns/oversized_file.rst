***************************
Files Longer Than 500 Lines
***************************

:Rule ID: ``oversized_file``
:Severity: high

Handwritten C++ source files must not exceed 500 counted lines.
Large files make responsibilities harder to identify, slow down review, and encourage unrelated changes to accumulate in
one compilation unit.

In ``.hpp`` files, standalone API-documentation lines beginning with ``///`` are excluded from the count.
This exception prevents the size rule from discouraging complete API documentation.
Lines containing declarations or definitions still count when they end in a trailing ``///<`` documentation comment.
Ordinary comments also count, as do all physical lines in ``.cpp`` and ``.tpp`` files.

Correct Solution
================

Split the file at logical responsibility boundaries.
For implementations of one class, name additional parts ``Class_part.cpp``, ``Class_part.hpp``, or ``Class_part.tpp`` as
described in the :doc:`file-structure guidelines <../cpp_files>`.
Do not compress formatting or remove useful documentation merely to meet the limit.

Generated Files
===============

Generated files are exempt because their structure is controlled by a generator and they are not reviewed or edited as
handwritten source.
The file must contain the case-insensitive text ``THIS IS A GENERATED FILE`` within its first ten physical lines.
Put the marker in the standard generated-file header and change the generator rather than adding it manually.

Mechanical Detection
====================

The scanner reports the 501st counted line of every non-generated ``cpp``, ``hpp``, or ``tpp`` file in the configured
source directories.
In headers, the reported physical line number can be higher because preceding standalone API documentation lines are
excluded.
