..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************
Generate Time Tests
*******************

``generate-time-tests`` generates eleven fixture tables under ``test/unittest/data/time``.
An independent Gregorian calendar model calculates expectations without using the date and time operations being tested.
The native command requires no Python interpreter or Python packages.

Check or Regenerate the Tables
==============================

Run these commands from the repository root:

.. code-block:: console

    ./erbsland-core-tool generate-time-tests --check
    ./erbsland-core-tool generate-time-tests

``--check`` (``-c``) fails for missing or different output without writing files or creating output directories.
Normal generation prepares every table before publishing any output.
Equivalent files retain their existing content and modification time; each changed file is replaced atomically.

Configuration and Overrides
===========================

The ``output_directory`` setting is in ``tool/conf/tools/generate_time_tests.elcl``.
Use ``--output-directory`` (``-o``) to override it for one invocation:

.. code-block:: console

    ./erbsland-core-tool generate-time-tests --output-directory staging/time

Output paths must remain inside the project.
The command validates configuration and destination conflicts before generating data.
Normal generation creates missing destination directories.
See :doc:`rendering_configuration` to customize generated notices.

Review Changes
==============

Random samples use local fixed-seed ``FastRandom`` instances.
Reproducibility is best effort across Core versions; explicit boundary cases remain fixed.
Review sample changes separately from changes to mathematical expectations.

Run the affected date and time suites, the native tool tests, and the repository pre-commit checks before accepting
changes.
Calendar model changes also require the exhaustive ``FullRun`` calendar test.
See :doc:`../tool_reference/time_test_generator_reference` for fixture coverage and the independent calculation model.
