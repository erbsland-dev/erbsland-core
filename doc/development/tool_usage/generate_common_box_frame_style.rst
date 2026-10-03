..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************************
Generate Common Box-Frame Style
*******************************

``generate-common-box-frame-style`` builds the terminal box-frame character attributes and lookup arrays from the
ordered repertoire in ``tool/data/cterm`` and verified names in the shared UCD snapshot.

Check or Regenerate
===================

Run from the repository root with the tool built from the current checkout:

.. code-block:: console

    ./erbsland-core-tool generate-common-box-frame-style --check
    ./erbsland-core-tool generate-common-box-frame-style

The output is ``src/erbsland/cterm/impl/CommonBoxFrameBlockCombinationStyle_data.cpp``.
``--check`` (``-c``) reports a missing or stale file without writing.
Review the generated arrays and run the terminal frame tests after updating it.

Configuration
=============

``tool/conf/tools/generate_common_box_frame_style.elcl`` sets ``output_file``.
Use ``--output-file`` (``-o``) for a one-off comparison in an existing project directory.
``tool/conf/source/box_frame.elcl`` selects the ordered repertoire.
See :doc:`../tool_reference/terminal_frame_generator_reference` for the source and selection contracts; see :doc:`usage`
for shared options.
