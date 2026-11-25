..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: BER for LDAP

************
BER for LDAP
************

Introduction
============

``BerParser`` and ``BerEncoder`` implement the wire types used in LDAP messages.
They support signed INTEGER, ENUMERATED, BOOLEAN, NULL, primitive OCTET STRING, SEQUENCE, SET OF, and implicit or
constructed application and context-specific tags.
Binary attribute and control values retain their original octets.

Lengths must be definite.
The decoder accepts long-form lengths with leading zeros or values below 128. The encoder writes the shortest definite
length.
Boolean true uses ``FF``.
SET OF preserves insertion order.
The LDAP schema determines field defaults, message structure and the meaning of implicitly tagged content.
See `RFC 4511 section 5.1 <https://www.rfc-editor.org/rfc/rfc4511.html#section-5.1>`_.

For stream input, pass the receive buffer to ``BerParser::frameLength()``.
An empty optional means more bytes are needed.
A malformed header or a declared frame exceeding the byte budget raises ``ParseError``.
When a length is available, pass that exact slice to ``parseOrThrow()`` and retain the remaining bytes for the next
frame.
The probe inspects the outer header.
Document parsing validates the enclosed values.

Interface
=========

.. doxygenclass:: erbsland::data::ber::BerEncoder
    :members:
.. doxygenclass:: erbsland::data::ber::BerParser
    :members:
