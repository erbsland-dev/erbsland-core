..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***************************
Generate Category Test Data
***************************

``generate-category-test-data`` regenerates ``test/unittest/data/re/CategoryContainsTestData.txt`` from the verified
Unicode snapshot under ``tool/data/ucd``.
It calculates regex category expectations independently of the library's category implementation and requires no Python
interpreter.

Check or Regenerate the Fixture
===============================

Run these commands from the repository root:

.. code-block:: console

    ./erbsland-core-tool generate-category-test-data --check
    ./erbsland-core-tool generate-category-test-data

``--check`` (``-c``) fails if the file is missing or differs, without writing files or creating directories.
Normal generation preserves equivalent files and atomically replaces changed output.
The header records the Unicode version and the command that produced the file.

The ``output_directory`` setting is in ``tool/conf/tools/generate_category_test_data.elcl``.
Use ``--output-directory`` (``-o``) to override it for one invocation; the destination must remain inside the project.
The shared ``[unicode]`` configuration selects the source directory and explicit version pin.
Source integrity and version checks must succeed before output is published.

Regenerate this fixture when :doc:`updating Unicode <update_unicode>` or changing regex category rules.
Run ``CategoryContainsTest`` and the native ``CategoryTestDataTest`` suite, including its ``FullRun`` tests.
See :doc:`../tool_reference/category_test_generator_reference` for the classification and sampling contract.
