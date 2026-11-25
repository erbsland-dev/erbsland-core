..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********************************
Terminal Frame Generator Reference
**********************************

``tool::cterm::CommonBoxFrameStyleTool`` and ``tool::cterm::FrameBorderJointCharsTool`` share one verified character
source and stroke model.
See :doc:`../tool_usage/generate_common_box_frame_style` and
:doc:`../tool_usage/generate_frame_border_joint_chars` for the command workflows.

Configuration and Source Contract
=================================

The required ``[generate_common_box_frame_style]`` and ``[generate_frame_border_joint_chars]`` sections each have an
``output_file`` text setting.
Their defaults are ``src/erbsland/cterm/impl/CommonBoxFrameBlockCombinationStyle_data.cpp`` and
``src/erbsland/cterm/impl/FrameBorder_data.hpp``.
``--output-file`` (``-o``) overrides the destination for one run; ``--check`` (``-c``) fails for missing or different
output without writing.

The ``[box_frame]`` section selects ``input_file``, defaulting to ``tool/data/cterm/box_frame_characters.txt``.
It lists one Unicode scalar per line in selection order.
Its sibling manifest records the line-ending-independent SHA3-256 hash.
``UnicodeSource`` verifies the shared UCD snapshot and supplies names from ``UnicodeData.txt``.
No Python Unicode database or compiled Core character table supplies selection facts.

Selection and Representation
============================

``BoxFrameData`` interprets the verified UCD names as seven stroke attributes: right, down, left, up, the two diagonals,
and center.
It preserves repertoire order.
Exact attribute matches use the last matching source character; scored matches keep the first character at an equal
score.
Combination favors an exact merged character and otherwise uses the established score and overlay threshold.

The common box-frame command emits source-order characters and attributes, exact lookup arrays, position weights, and
special character indexes.
The frame-border command selects a 5 by 5 by 5 by 5 joint table, with named round-corner characters from the same
source.
Render templates under ``tool/lib/core/resources/cterm`` own both C++ layouts.

Verification
============

``BoxFrameToolTest`` checks UCD name interpretation, combination, ordered ties, malformed repertoire entries,
source-hash failures, and both commands' check mode.
The affected Core terminal tests verify runtime selection.
