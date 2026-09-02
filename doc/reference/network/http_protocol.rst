..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: HTTP; Reference
    single: HTTP values
    single: HTTP headers
    single: media type
    single: HTTP message heads
    single: HTTP/1.1 codec
    single: malformed UTF-8; HTTP headers

*******************
HTTP Protocol Types
*******************

HTTP Value Types
================

Methods, Versions, and Statuses
-------------------------------

``HttpMethod`` stores any valid case-sensitive HTTP method token.
Canonical uppercase spellings of the common standard methods are classified with ``HttpMethodType`` and can be grouped
in ``HttpMethodTypes``.
Extension methods retain their exact spelling and remain exactly comparable.

``HttpVersion`` represents HTTP/1.0 and HTTP/1.1. ``HttpStatus`` accepts every numeric code from 100 through 599 and
provides named constants and conventional reason phrases for recognized codes.
Reason phrases are conveniences; the HTTP codec owns status-line syntax.

Media Types
-----------

``HttpMediaType`` stores canonical lowercase type and subtype tokens plus ordered ``HttpMediaTypeParameter`` values.
Parameter names are case-insensitive and unique.
Parsing accepts token and quoted-string values, removes quoting and escapes, and canonical output quotes a semantic
value only when token syntax cannot represent it.

Header Fields
-------------

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

HTTP Messages and Codecs
========================

Message-Head Values
-------------------

``HttpRequestHead`` and ``HttpResponseHead`` are validated, read-only message-head values.
A default-constructed value is an invalid placeholder.
A successful constructor validates the start-line components and all header fields.

A request head stores the method, exact printable-ASCII request target, HTTP version, and headers.
A response head stores the HTTP version, status, exact reason phrase, and headers.
Both HTTP/1.0 and HTTP/1.1 are supported.

Raw Field Semantics
-------------------

HTTP field values are stored as ``String`` values without normalization or transcoding.
Parsing removes only field-line edge optional whitespace, meaning space and horizontal-tab bytes immediately after the
colon or at the end of the field line.
Every interior byte is retained, including ``obs-text`` and malformed UTF-8.

The string decoder represents each malformed input byte as a replacement character while preserving its original native
byte storage.
Consequently, the regular character API is safe to use, and reserialization writes the original bytes unchanged.
Encoders use canonical ``Name: value`` framing and never require a parallel byte-oriented field API.

Incremental HTTP/1.x Codecs
---------------------------

The HTTP/1.x request and response codecs are internal implementation components.
They are transport-independent and callback-free.
Input is accepted incrementally into a bounded queue, and decoding yields message-head, body, trailers, and completion
events.
End-of-input is signalled explicitly.
Completed codecs can reset while retaining pipelined input; protocol-switch and successful ``CONNECT`` responses expose
the remaining bytes as opaque data.

Parsing is strict: lines require CRLF, start-line spacing is exact, obsolete folding and whitespace before a field colon
are rejected, and framing ambiguities are errors.
Repeated or comma-separated ``Content-Length`` values are accepted only if every value is valid and identical.
``Content-Length`` together with ``Transfer-Encoding`` is always rejected.
Chunked bodies are dechunked, chunk extensions are validated and discarded, and trailers remain separate from the
initial headers.
Only unclassified extension fields are permitted in trailers.

Response framing applies the special rules for ``HEAD``, informational responses, ``101``, ``204``, ``304``, and
successful ``CONNECT``.
Apart from final ``chunked``, transfer codings and compression are not implemented by these codecs.

Limits
------

Default limits are 8 KiB for a start line, the configured ``HttpHeaderLimits`` separately for headers and trailers, 8
KiB for each chunk-metadata line, 16 MiB for a decoded body, 16 KiB for yielded body chunks, and 1 MiB for each retained
input or output queue.
Limits are checked before attacker-controlled allocation.

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
.. doxygenclass:: erbsland::network::HttpRequestHead
    :members:
.. doxygenclass:: erbsland::network::HttpResponseHead
    :members:
.. doxygenclass:: erbsland::network::HttpStatus
    :members:
.. doxygenclass:: erbsland::network::HttpVersion
    :members:
