..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Overview

***********************
Text Rendering Overview
***********************

When a document has stable wording but needs conditions, repeated sections, or values supplied at runtime, a layout
keeps its structure separate from its data.
These topics lead from a first render call to the language used inside a layout.

Start with :doc:`rendering_text_layouts` for the renderer's purpose and basic terminology.
:doc:`using_the_render_engine` follows a complete workflow from finding a layout to rendering it with a local context.

:doc:`configuring_the_render_environment` explains shared values, filters, syntax, escaping, and limits.
:doc:`loading_render_layouts` shows how to load layouts from files, compiled resources, or an application source.
:doc:`preparing_render_contexts` explains named values, nested data, and lazy callbacks for a render call.

:doc:`template_language_syntax` shows how expressions, filters, conditions, and loops shape the text written by a
layout.
:doc:`blocks_and_inheritance` shows how named blocks let related layouts share a document structure while changing
selected passages.
Keep :doc:`template_feature_cheat_sheet` open while writing layouts for a compact list of supported forms.
