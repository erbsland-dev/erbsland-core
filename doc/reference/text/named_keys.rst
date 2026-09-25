..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Named-Key Parsing; Reference

*****************
Named-Key Parsing
*****************

:cpp:class:`Format <erbsland::text::named_key::Format>` describes the aliases, separators, value policy, and limits of
a named-key entry list.
:cpp:class:`Parser <erbsland::text::named_key::Parser>` reads from a
:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` and returns
:cpp:class:`Entry <erbsland::text::named_key::Entry>` values. Entries may carry a recognized key, a keyed value, or a
positional value.
Invalid syntax raises a positioned ``err::ParseError``.

The format must outlive its parser.
Register normalized aliases with semantic integer identifiers before reading.
``readEntry()`` returns a terminal entry at the end; ``readAllEntries()`` excludes that entry.

Interface
=========

.. doxygenclass:: erbsland::text::named_key::Entry
    :members:
.. doxygenenum:: erbsland::text::named_key::EntryKind
.. doxygenclass:: erbsland::text::named_key::Format
    :members:
.. doxygenclass:: erbsland::text::named_key::Parser
    :members:
