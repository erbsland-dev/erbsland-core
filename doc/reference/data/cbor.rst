..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: CBOR Values

***********
CBOR Values
***********

Introduction
============

``CborValue`` stores a CBOR value tree and reads or writes one complete data item.
``CborParseOptions`` sets input limits and the DAG-CBOR profile; ``CborFormatOptions`` selects the output profile.
See :doc:`/topics/data/reading_and_writing_cbor` for examples, supported types, tags, navigation, and wire rules.

Truncated input is rejected as ``ParseError`` by the throwing decoder, or as an empty optional by the tolerant decoder.
Internal bounds errors and resource failures are not converted into malformed-input errors.

Text arguments accept ``String``, ``StringEditor``, and Core string literals directly, including when passed to
``set()`` or ``append()``.
Stored text retains its value if the original editor is changed later.

Interface
=========

.. doxygentypedef:: erbsland::data::cbor::CborArray

.. doxygentypedef:: erbsland::data::cbor::CborMap
.. doxygenclass:: erbsland::data::cbor::CborFormatOptions
    :members:
.. doxygenstruct:: erbsland::data::cbor::CborLink
    :members:
.. doxygenclass:: erbsland::data::cbor::CborParseOptions
    :members:
.. doxygenenum:: erbsland::data::cbor::CborType
.. doxygenclass:: erbsland::data::cbor::CborValue
    :members:
