.. index::
    single: Terminal Document Renderer

**************************
Terminal Document Renderer
**************************

Introduction
============

The terminal document renderer writes ``text::TextDocument`` and ``text::TextNode`` trees as styled terminal text.
``TerminalDocumentStyle`` describes text style overlays, paragraph layout, decorations, and list markers with selectors
that match ``text::TextNodeType`` values directly.

Usage
=====

Use ``TerminalDocumentRenderer`` when semantic document nodes should be rendered to a ``CursorWriter``.
The renderer keeps the current style by value, so a renderer can be reused for repeated render calls.

The writer width selects the layout width once at the start of a render.
Reported widths of 60 columns or more are used directly.
Widths from 1 through 59, an unknown width, and zero select an 80-column layout.
A narrower destination may wrap these completed lines naturally.
There is no separate narrow-output layout.

Use ``TerminalDocumentStyle`` to customize the default text style, base block layout, and selector-specific rules.
Rules use ``BlockStyle`` for terminal text style overlays and ``ParagraphIndents`` with ``block::Margins`` for
layout.
Prefixes, suffixes, and markers are exposed as read-only ``BlockString`` values.

Selectors may constrain a rule to nodes below an ancestor node type.
Ancestor matching considers the complete parent chain, which allows a diagnostic title inside a block quote to differ
from the top-level title.

Container Decorations
=====================

For structural containers, ``prefix`` and ``suffix`` are standalone lines outside the content.
``linePrefix`` is prepended to each logical content line.
Nested containers combine their line prefixes from outermost to innermost, which is useful for reply-style quote
borders.

Structured Fields and Inline Paths
==================================

``FieldList`` contains ``FieldItem`` nodes made from ``FieldLabel`` and ``FieldContent``.
Labels occupy an aligned column while they fit within one quarter of the available width.
Unstacked content begins one column after the widest rendered label, including its trailing colon.
If any label exceeds that threshold, the whole list uses stacked values with an eight-column continuation indent.
Nested field lists add four columns of indentation.

``Separator`` and ``EscapeSequence`` are generic inline semantic nodes.
Separators are styled wrapping opportunities that remain on the preceding line; escape sequences remain indivisible.
These semantics are represented as source-relative metadata.
The renderer does not insert hidden Unicode characters into the text.

Consecutive inline children of a list item form one semantic paragraph.
Inline style boundaries such as ``Strong`` or ``Emphasis`` therefore do not introduce physical line breaks; only block
children and explicit line-break nodes do.

The default system-output style renders cause headings and their frame as non-bold bright black.
When terminal styling is unavailable, the plain renderer preserves diagnostic structure with ``Error:`` on the root,
colons on headings, two-space content indentation, nested cause indentation, and ``│`` code gutters.

Implementation Notes
====================

The renderer has one forward data flow:

.. code-block:: text

    TextDocument
        -> semantic inline content
        -> logical blocks
        -> styled physical lines
        -> CursorWriter

The inline builder traverses each text-bearing node in ``prefix``, direct text, children, ``suffix`` order.
Alongside the styled text it records soft-break boundaries for ``Separator`` nodes and indivisible source ranges for
``EscapeSequence`` nodes.

The block builder resolves selectors, text styles, margins, paragraph indents, list markers, and the ordered path of
active container frames.
A logical block stores only these resolved values; it does not retain mutable style rules or output-specific offsets.

The document layout owns width calculation, vertical margin collapse, paragraph wrapping, field columns, and frame
composition.
Paragraph content is wrapped at the width remaining after frame and child margins.
Each finished content line is then composed in this order: outer container spacing, ordered frame prefixes, child
margin, paragraph or list indentation, and content.
Blank lines inside a frame use the active frame prefix, which keeps borders continuous.
Paragraph line construction uses the same internal paragraph ``Printer`` as direct cursor paragraph output; buffer
painting remains fragment-based because its colors may depend on target cells.

Only after every physical line has been materialized does the writer adapter emit output.
It performs no layout and writes exactly one styled line plus one line break per physical line.
New block kinds can therefore extend the block builder and physical-line materializer without adding another sink or
traversal mode.

The core invariants are:

* layout width is selected once per render;
* semantic indices always refer to unmodified inline source text;
* frames and margins never participate in paragraph tokenization;
* vertical margin collapse occurs only between logical blocks;
* output starts only after all physical lines have been built.

Interface
=========

.. doxygenclass:: erbsland::cterm::TerminalDocumentRenderer
    :members:
.. doxygenclass:: erbsland::cterm::TerminalDocumentStyle
    :members:
.. doxygenclass:: erbsland::cterm::TerminalDocumentStyleMarker
    :members:
.. doxygenclass:: erbsland::cterm::TerminalDocumentStyleRule
    :members:
.. doxygenclass:: erbsland::cterm::TerminalDocumentStyleSelector
    :members:
