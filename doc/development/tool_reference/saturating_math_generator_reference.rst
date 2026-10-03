..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************************
Saturating Math Generator Reference
***********************************

``tool::test_data::SaturatingMathTool`` generates reviewed arithmetic boundary fixtures.
See :doc:`../tool_usage/generate_saturating_math_data` for the regeneration workflow.

Configuration and Options
=========================

The required ``[generate_saturating_math_data]`` section has a text setting ``output_directory``, defaulting to
``test/unittest/data/saturating_math``.
Its checked-in configuration is ``tool/conf/tools/generate_saturating_math_data.elcl``.
``--output-directory`` (``-o``) overrides the setting; ``--check`` (``-c``) compares without writing or creating
directories.
Paths must remain inside the project.
Configuration, path types, and all six destination paths are validated before generation.

Independent Arithmetic
======================

``impl::SaturatingMathData`` caches the boundary samples, bounds, and decimal representations for the signed and
unsigned 8-, 16-, 32-, and 64-bit types.
It uses ``BigInteger`` for exact calculations and explicit comparisons with target bounds.
Expectations do not call the saturating operations being tested.

The six tables preserve the reviewed boundary sample sets and type ordering.
Division truncates toward zero; remainders retain the dividend's sign.
Zero divisors are excluded.
The signed minimum divided by minus one saturates to the maximum, while its modulo result is zero.

Output and Rendering
====================

``cast.txt`` contains target type, source type, input, expected output, and an overflow flag.
``add.txt``, ``subtract.txt``, ``multiply.txt``, ``divide.txt``, and ``modulo.txt`` contain the operand types, operand
values, expected result, and an overflow flag; each file's schema header defines the field order.
The shared ``test_data/table.tpl`` template supplies notices, schema headers, and row iteration.
Native code supplies structured render values, representing numbers outside signed 64-bit range as decimal strings.

All tables are prepared before publication.
``ProjectFiles::updateText()`` retains equivalent existing files and atomically replaces each changed file.
An operating-system failure during publication can leave previously published files updated.
See :doc:`framework_reference` for comparison and path contracts and :doc:`rendering_reference` for notice
configuration.

Verification
============

``FixtureGeneratorsTest`` covers boundary calculations, repeatability, options, configuration precedence, confined
paths, destination conflicts, check mode, and preservation of equivalent CRLF files.
Generation and integration tests use the ``FullRun`` tag; see
:ref:`Run the tool tests <development-tool-setup-run-the-tool-tests>`.
