.. index::
    single: Diagnostics

***********
Diagnostics
***********

Diagnostic documents
====================

Diagnostics convert to neutral :cpp:class:`TextDocument <erbsland::text::TextDocument>` trees.
Use :cpp:func:`Diagnostic::toTextDocument() <erbsland::err::Diagnostic::toTextDocument>` for one diagnostic or
``diagnosticDocumentFromException()`` for a root diagnostic with causes, then render the result with the plain or
terminal document renderer.
Line-oriented excerpts use the universal :cpp:class:`CodeSnippetMarker <erbsland::text::CodeSnippetMarker>` and
``TextNodeType::CodeSnippet`` nodes from the text domain.

Diagnostic metadata is represented with ``FieldList`` nodes so labels remain secondary to their values.
Related field lists are introduced by a diagnostic section heading; each list aligns its own labels and values.
Causes are represented recursively as sibling pairs: a “Caused By” heading with the ``diagnostic-cause`` style is
followed by a ``Blockquote`` with the same style.
The quote contains the diagnostic followed by the next heading and quote pair when present.
This structure lets terminal styles draw a visible border for every cause depth while keeping only the root diagnostic
title in the primary error color.
The cause heading and its frame use a subdued bright-black style so they read as structure rather than another error
title.

Developer-authored diagnostic contexts and display text are trusted.
External values such as paths, command-line arguments, native messages, and foreign exception text are represented by
escaped semantic nodes before rendering.

Plain rendering prefixes the root title with ``Error:``, adds colons to section headings, indents ordinary section
content by two spaces, and adds another two spaces for each cause depth.
Code snippets retain compact indentation and use ``│`` as their gutter so redirected diagnostics remain structured.

Interface
=========

.. doxygenclass:: erbsland::err::Diagnostic
    :members:

.. doxygentypedef:: erbsland::err::DiagnosticPtr

.. doxygentypedef:: erbsland::err::DiagnosticConstPtr
.. doxygenclass:: erbsland::err::DiagnosticHelper
    :members:
.. doxygenclass:: erbsland::err::ErrorDocumentBuilder
    :members:
