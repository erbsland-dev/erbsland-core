..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********************
Generate Display Texts
**********************

``generate-display-texts`` updates Core's checked-in base display-text header and implementation from its ELCL catalog.
Run it after changing the catalog or its generator; ordinary library builds use the checked-in files without
regenerating them.
See :doc:`usage` for shared invocation options and :doc:`../setup` for installation.

Edit and Regenerate the Catalog
===============================

Edit ``src/erbsland/i18n/impl/DisplayTextCore.elcl``, then regenerate the C++ files from the repository root:

.. code-block:: console

    ./erbsland-core-tool generate-display-texts

The default outputs are ``DisplayTextCore.hpp`` and ``DisplayTextCore.cpp`` beside the catalog.
Review their diff and commit both generated files with the ELCL change.
Do not edit generated files directly.
For catalog syntax, see :doc:`/reference/i18n/internationalization`.

Check Without Writing
=====================

To verify that both outputs are current:

.. code-block:: console

    ./erbsland-core-tool generate-display-texts --check

``--check`` (``-c``) fails when either output is missing or differs, without changing files.
If the diagnostic reports stale output, run without ``--check`` and review the result.
Normal generation preserves unchanged files.

Choose Inputs and Outputs
=========================

The command's settings are in ``tool/conf/tools/generate_display_texts.elcl``, included by the main configuration.
Change this file to select another catalog, output paths, class name, or namespace.
Paths are relative to the project root, and output directories must already exist.
There are no command-line path overrides.
See :doc:`../tool_reference/display_text_generator_reference` for every setting and validation rule, and
:doc:`rendering_configuration` for generated notices.
