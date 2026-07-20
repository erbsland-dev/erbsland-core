.. index::
    single: String Splitter
    single: splitting strings

****************
String Splitters
****************

String splitters read an owning Core string sequentially and return copy-on-write slices without copying their text.
``StringSplitter`` is the common UTF-8 alias.
The ``U8StringSplitter``, ``U16StringSplitter`` and ``U32StringSplitter`` variants provide the same interface for each
supported width.

Separators
==========

A splitter accepts either one :cpp:class:`Char <erbsland::text::Char>` or a
:cpp:class:`CharSet <erbsland::text::CharSet>`. Each call to ``next()`` reads through the next matching separator.
Malformed encoded data is handled tolerantly like the underlying string type.

:cpp:enumerator:`StringSplitMode::DiscardSeparator <erbsland::text::StringSplitMode::DiscardSeparator>` returns only
the text between separators.
Consecutive separators therefore return empty parts, and a trailing separator produces a final empty part.
:cpp:enumerator:`StringSplitMode::KeepSeparator <erbsland::text::StringSplitMode::KeepSeparator>` includes the
separator at the end of each part.
In this mode a trailing separator completes the preceding part without producing another empty part, which is useful for
line-oriented processing.

Sequential State
================

``isAtEnd()`` distinguishes an empty part from the end of the sequence.
Calling ``next()`` after the end safely returns an empty string.
``remaining()`` returns the unread suffix as another shared slice, and ``reset()`` restarts the splitter at the
beginning.

An empty source has one empty part.
This preserves ordinary split semantics and lets a caller observe the source once before ``isAtEnd()`` becomes true.

The UTF-8 implementation has a direct byte-search path for a single ASCII separator.
ASCII bytes cannot occur inside a UTF-8 multibyte sequence, so this optimization preserves tolerant decoding behavior.
Other separator sets use decoded character matching.
The width-specific backends can use independent search optimizations while keeping the same public contract.

Interface
=========

.. doxygenenum:: erbsland::text::StringSplitMode
.. doxygentypedef:: erbsland::text::StringSplitter
.. doxygentypedef:: erbsland::text::U16StringSplitter
.. doxygentypedef:: erbsland::text::U32StringSplitter
.. doxygentypedef:: erbsland::text::U8StringSplitter
