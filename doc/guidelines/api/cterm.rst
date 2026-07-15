Color Terminal Domain API Guidelines
====================================

These guidelines extend the Common and Block Geometry API Guidelines for public APIs in the ``cterm`` domain.

Core Semantics
==============

- The terminal screen is modeled as a grid of blocks.
- A ``Block`` is one terminal cell worth of visible content plus its style.
- Compared to ``Char``, a block can represent multiple Unicode code-points.

Naming
------

.. code-block:: text

    Block... // APIs that operate on terminal cells
        BlockString[View] // styled terminal-cell strings
        BlockText // renderable text descriptions
    Terminal... // APIs that make use of terminal input/output

Print
-----

.. code-block:: text

    print[Line](args...) // mixed styled terminal output


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
    TerminalDocumentRenderer // renderer for text::TextDocument trees
    TerminalDocumentStyle // selector-driven style sheet for terminal documents
    TerminalDocumentStyleSelector // selector for document style rules
    TerminalDocumentStyleRule // text/layout/decorations for one document style rule
    TerminalDocumentStyleMarker // list marker definition for terminal documents

TerminalDocument... Patterns
============================

TerminalDocumentRenderer
------------------------

.. code-block:: text

    T([style]) // render a text document using the plain style
    T::defaultPlain() // compact uncolored document style
    T::defaultSystemOutput() // semantic style for application help, option errors and diagnostics
    renderer.renderTo(writer, document) // write completed styled physical lines to a cursor writer
    width >= 60 // layout at the reported writer width
    width < 60 or unknown // layout at 80 columns; the destination may wrap naturally

TerminalDocumentStyle
---------------------

.. code-block:: text

    T::defaultStyle(style) -> T // get default style enum based
    T::default...() -> T // get an individual default style.
