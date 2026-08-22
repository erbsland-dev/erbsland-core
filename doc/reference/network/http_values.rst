.. index::
    single: HTTP values
    single: HTTP headers
    single: media type

****************
HTTP Value Types
****************

Methods, Versions, and Statuses
===============================

``HttpMethod`` stores any valid case-sensitive HTTP method token.
Canonical uppercase spellings of the common standard methods are classified with ``HttpMethodType`` and can be grouped
in ``HttpMethodTypes``.
Extension methods retain their exact spelling and remain exactly comparable.

``HttpVersion`` represents HTTP/1.0 and HTTP/1.1. ``HttpStatus`` accepts every numeric code from 100 through 599 and
provides named constants and conventional reason phrases for recognized codes.
Reason phrases are conveniences; the HTTP codec owns status-line syntax.

Media Types
===========

``HttpMediaType`` stores canonical lowercase type and subtype tokens plus ordered ``HttpMediaTypeParameter`` values.
Parameter names are case-insensitive and unique.
Parsing accepts token and quoted-string values, removes quoting and escapes, and canonical output quotes a semantic
value only when token syntax cannot represent it.

Header Fields
=============

``HttpFieldName`` retains the supplied ASCII spelling while comparison and hashing use ASCII case folding.
``HttpFieldType`` recognizes the HTTP core fields plus Cookie and Set-Cookie; arbitrary extension names remain valid.
Request and response applicability is descriptive metadata, not a rejection rule.

``HttpField`` keeps the exact supplied field value, including obs-text and malformed UTF-8 bytes.
Leading or trailing field-line optional whitespace (SP or HTAB), CR, LF, NUL, DEL, and other prohibited control bytes
are rejected at construction.
The HTTP codec removes field-line edge OWS before constructing this value; every interior byte is retained without UTF-8
validation, normalization, or transcoding.

``HttpHeaders`` is an ordered copy-on-write field list.
Repeated fields remain separate and ordered.
Every mutation enforces captured ``HttpHeaderLimits`` and has a strong failure guarantee.
``setField()`` replaces the first matching position, removes later duplicates, and appends only when the field was
absent.

The default limits are 100 fields, 256 bytes per name, 16 KiB per value, and 64 KiB aggregate serialized size.
Aggregate accounting uses the canonical ``name: value`` form plus CRLF for each field.

Interface
=========

.. doxygenenum:: erbsland::network::HttpCookieSameSite
.. doxygenclass:: erbsland::network::HttpField
    :members:
.. doxygenclass:: erbsland::network::HttpFieldName
    :members:
.. doxygenclass:: erbsland::network::HttpFieldType
    :members:
.. doxygenclass:: erbsland::network::HttpHeaderLimits
    :members:
.. doxygentypedef:: erbsland::network::HttpFieldList

.. doxygenclass:: erbsland::network::HttpHeaders
    :members:
.. doxygentypedef:: erbsland::network::HttpMediaTypeParameters

.. doxygenclass:: erbsland::network::HttpMediaType
    :members:
.. doxygenclass:: erbsland::network::HttpMediaTypeParameter
    :members:
.. doxygenclass:: erbsland::network::HttpMethod
    :members:
.. doxygenenum:: erbsland::network::HttpMethodType

.. doxygentypedef:: erbsland::network::HttpMethodTypes
.. doxygenclass:: erbsland::network::HttpStatus
    :members:
.. doxygenclass:: erbsland::network::HttpVersion
    :members:
