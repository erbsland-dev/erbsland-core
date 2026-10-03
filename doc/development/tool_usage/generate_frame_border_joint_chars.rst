..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**************************************
Generate Frame-Border Joint Characters
**************************************

``generate-frame-border-joint-chars`` selects terminal frame joints from the ordered repertoire in ``tool/data/cterm``
and verified names in the shared UCD snapshot.

Check or Regenerate
===================

Run from the repository root with the tool built from the current checkout:

.. code-block:: console

    ./erbsland-core-tool generate-frame-border-joint-chars --check
    ./erbsland-core-tool generate-frame-border-joint-chars

The output is ``src/erbsland/cterm/impl/FrameBorder_data.hpp``.
``--check`` (``-c``) reports a missing or stale file without writing.
Review the selected joints and run the terminal frame tests after updating it.

Configuration
=============

``tool/conf/tools/generate_frame_border_joint_chars.elcl`` sets ``output_file``.
Use ``--output-file`` (``-o``) for a one-off comparison in an existing project directory.
``tool/conf/source/box_frame.elcl`` selects the ordered repertoire.
See :doc:`../tool_reference/terminal_frame_generator_reference` for the source and selection contracts; see :doc:`usage`
for shared options.
