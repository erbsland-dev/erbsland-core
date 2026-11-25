..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Distinguished Encoding Rules

****************************
Distinguished Encoding Rules
****************************

Introduction
============

``DerParser::parse()`` reads exactly one DER value.
``parseOrThrow()`` reports malformed input and trailing bytes with a document-relative byte index.
The codec supports the primitive and constructed types used by Core certificates and keys, signed INTEGER, and
ENUMERATED.
The ASN.1 schema determines implicit types, required fields, DEFAULT omission and the interpretation of time values.

SET and SET OF share universal tag 17. The parser accepts either valid ordering.
Schema decoders can use ``isSetOrdered()`` to require ascending tags or ``isSetOfOrdered()`` to require ascending
complete encodings.

``DerEncoder`` writes typed values into a shared output block.
Close each container in reverse opening order.
Use ``beginSet()`` for tag-ordered components, ``beginSetOf()`` for already ordered elements, or ``appendSetOf()`` to
sort individually encoded elements.
``appendEncoded()`` accepts a DER-parsed node or validates a complete byte block.
``markAsSensitive()`` marks the output storage for key material.

The wire rules are defined in `ITU-T X.690 <https://www.itu.int/rec/T-REC-X.690/en>`_.

Interface
=========

.. doxygenclass:: erbsland::data::der::DerEncoder
    :members:
.. doxygenclass:: erbsland::data::der::DerParser
    :members:
