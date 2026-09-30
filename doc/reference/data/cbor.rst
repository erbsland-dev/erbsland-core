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
