.. index::
    single: Text Document
    single: Text Node

*************
Text Document
*************

Semantic Escaped Text
=====================

Use :cpp:func:`TextNode::addEscapedText() <erbsland::text::TextNode::addEscapedText>` to insert untrusted text into a
document.
The method tolerantly decodes malformed input, groups ordinary characters into ``Text`` nodes, and creates one
indivisible ``EscapeSequence`` node for every escaped character.
This preserves wrapping and styling semantics while preventing raw control sequences from reaching a renderer.

Interface
=========

.. doxygenstruct:: erbsland::text::CodeSnippet
    :members:
.. doxygenclass:: erbsland::text::CodeSnippetMarker
    :members:

.. doxygentypedef:: erbsland::text::CodeSnippetMarkerList
.. doxygenclass:: erbsland::text::PlainTextRenderer
    :members:
.. doxygenclass:: erbsland::text::TextDocument
    :members:
.. doxygenclass:: erbsland::text::TextNode
    :members:
.. doxygenclass:: erbsland::text::TextNodeData
    :members:
.. doxygenclass:: erbsland::text::TextNodeType
    :members:
.. doxygenclass:: erbsland::text::TextWalkResult
    :members:
.. doxygenenum:: erbsland::text::TextWalkStatus
