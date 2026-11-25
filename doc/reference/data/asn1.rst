..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: ASN.1 Values

************
ASN.1 Values
************

Introduction
============

An ``Asn1Node`` retains the exact encoded bytes of a parsed value.
A child, an encoded slice, or a content slice can outlive the original input and the parent node.
Byte positions are relative to the start of the parsed document.
Parsing an embedded DER value with ``DerParser::parseContentOrThrow()`` preserves the enclosing document's positions.

Use ``matches()`` to test the tag and primitive or constructed form together.
Typed accessors match the universal tag by default.
Supply an application or context-specific tag when the schema uses implicit tagging.
Integer accessors return an empty optional when the value exceeds the native type.
BIT STRING access includes the unused-bit count.
An empty OCTET STRING is a present value with zero bytes.

The decoder enforces the byte, depth and node budgets in ``Asn1Limits``.
The defaults are 16 MiB, 32 levels and 8,192 nodes.
The root counts as one level and one node.
The implementation supports at most 32 levels.

Interface
=========

.. doxygenstruct:: erbsland::data::asn1::Asn1BitString
    :members:
.. doxygenstruct:: erbsland::data::asn1::Asn1Limits
    :members:
.. doxygenclass:: erbsland::data::asn1::Asn1Node
    :members:
.. doxygenclass:: erbsland::data::asn1::Asn1ObjectIdentifier
    :members:
.. doxygenstruct:: erbsland::data::asn1::Asn1Tag
    :members:
.. doxygenenum:: erbsland::data::asn1::Asn1TagClass
.. doxygenenum:: erbsland::data::asn1::Asn1UniversalType
