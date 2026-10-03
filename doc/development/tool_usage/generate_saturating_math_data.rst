..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************************
Generate Saturating Math Data
*****************************

``generate-saturating-math-data`` generates the six arithmetic fixture tables under
``test/unittest/data/saturating_math``.
It uses exact integer arithmetic to calculate expectations independently of the saturating operations being tested.
The native command requires no Python interpreter or Python packages.

Check or Regenerate the Tables
==============================

Run these commands from the repository root:

.. code-block:: console

    ./erbsland-core-tool generate-saturating-math-data --check
    ./erbsland-core-tool generate-saturating-math-data

``--check`` (``-c``) fails for missing or different output without writing files or creating output directories.
Normal generation prepares all six tables before publishing any output.
Equivalent files retain their existing content and modification time; each changed file is replaced atomically.

Configuration and Overrides
===========================

The ``output_directory`` setting is in ``tool/conf/tools/generate_saturating_math_data.elcl``.
Use ``--output-directory`` (``-o``) to override it for one invocation:

.. code-block:: console

    ./erbsland-core-tool generate-saturating-math-data --output-directory staging/saturating_math

Output paths must remain inside the project.
The command validates configuration and destination conflicts before generating data.
Normal generation creates missing destination directories.
See :doc:`rendering_configuration` to customize generated notices.

Review Changes
==============

Review the generated diff, run the affected saturating-math suites and native tool tests, and run the repository
pre-commit checks before accepting changes.
The reviewed boundary samples, ordering, and expected values should remain stable unless their coverage is deliberately
revised.
See :doc:`../tool_reference/saturating_math_generator_reference` for the calculation model and fixture schemas.
