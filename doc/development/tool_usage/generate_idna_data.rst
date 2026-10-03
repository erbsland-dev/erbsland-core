..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******************
Generate IDNA Data
******************

``generate-idna-data`` generates the compact IDNA2008 runtime table from the configured Unicode source snapshot in
``tool/data/ucd``.
Use it to check that the generated source is current or to regenerate it after a generator change.
Follow :doc:`repository setup <../setup>` if the maintenance tool is not installed yet.

.. _development-tool-generate_idna_data-check-or-regenerate-the-table:

Check or Regenerate the Table
=============================

Run these commands from the repository root.
The first checks without writing; the second updates the generated source when its contents differ.

.. code-block:: console

    ./erbsland-core-tool generate-idna-data --check
    ./erbsland-core-tool generate-idna-data

``--check`` (short form ``-c`` ) returns failure if the output is missing or differs.
It preserves existing output.
Normal generation leaves equivalent files untouched; line terminators and first-line copyright years are ignored.
On Windows, use ``tool\bin\erbsland-core-tool.exe`` in place of the root shortcut.

.. _development-tool-generate_idna_data-configuration-and-overrides:

Configuration and Overrides
===========================

Output settings are in ``tool/conf/tools/generate_idna_data.elcl``.
The default destination is ``src/erbsland/text/punycode/impl/IdnaData.cpp``.
For a one-off comparison, choose an output in an existing project directory:

.. code-block:: console

    ./erbsland-core-tool generate-idna-data --output-file tool/data/IdnaData.cpp

Shared Unicode inputs are selected in ``tool/conf/source/unicode.elcl``.
Keep the configured version pin and the source snapshot together; changing the version string alone does not update
data.
There are no command-line input overrides.
See :doc:`../tool_reference/idna_generator_reference` for all settings and input validation, and
:doc:`rendering_configuration` for generated notices.

.. _development-tool-generate_idna_data-update-the-unicode-dataset:

Update the Unicode Dataset
==========================

Follow :doc:`update_unicode` to acquire a consistent release and update its integrity metadata and version pin.
For derivation, table representation, and conformance tests, see :doc:`../tool_reference/idna_generator_reference`.
