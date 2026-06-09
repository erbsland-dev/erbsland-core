Color Terminal Domain API Guidelines
====================================

These guidelines extend the Common and Block Geometry API Guidelines for public APIs in the ``cterm`` domain.

Terminology
===========

The terminal screen is modeled as a grid of blocks.
A ``Block`` is one terminal cell worth of visible content plus its style.
This avoids using ``Char`` for terminal cells, because the text domain reserves ``Char`` for one Unicode code point.

Primary Types
=============

.. code-block:: text

    Block // one styled terminal cell
    BlockString // an owning sequence of terminal blocks
    BlockStringView // a shared read-only view into block string data
    BlockText // renderable block-string content plus rectangle and layout options
    BlockTextOptions // reusable options for rendering block text
    BlockStyle // color and attributes for a block
    BlockAttributes // terminal attributes for a block
    BlockPrintContext // shared implementation interface for print-style block output
    TerminalStream // text stream adapter for terminal output
    TerminalStreamSynchronization // shared synchronization state for terminal streams
    TerminalOptionsRenderer // terminal renderer for command line option output
    TerminalOptionsTheme // style set for terminal option output

Naming Patterns
===============

Use ``Block`` for APIs that operate on terminal cells.
Use ``BlockString`` for styled terminal-cell strings and ``BlockText`` for renderable text descriptions.
Avoid unqualified ``Char``, ``String``, and ``Text`` names in the ``cterm`` public API unless they refer to the text
domain or the C++ standard library.

Output Patterns
===============

Use ``print(args...)`` and ``printLine(args...)`` for mixed styled terminal output.
If multiple writers need the same print argument surface, route the variadic public API through ``BlockPrintContext``
instead of duplicating per-argument dispatch overloads.
Use ``TerminalStream`` when terminal output has to pass through the stream API.
Use a separate theme value type for terminal renderers when colors or attributes may become user-customizable.
