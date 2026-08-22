.. index::
    single: HTTP message heads
    single: HTTP/1.1 codec
    single: malformed UTF-8; HTTP headers

************************
HTTP Messages and Codecs
************************

Message-Head Values
===================

``HttpRequestHead`` and ``HttpResponseHead`` are validated, read-only message-head values.
A default-constructed value is an invalid placeholder.
A successful constructor validates the start-line components and all header fields.

A request head stores the method, exact printable-ASCII request target, HTTP version, and headers.
A response head stores the HTTP version, status, exact reason phrase, and headers.
Both HTTP/1.0 and HTTP/1.1 are supported.

Raw Field Semantics
===================

HTTP field values are stored as ``String`` values without normalization or transcoding.
Parsing removes only field-line edge optional whitespace, meaning space and horizontal-tab bytes immediately after the
colon or at the end of the field line.
Every interior byte is retained, including ``obs-text`` and malformed UTF-8.

The string decoder represents each malformed input byte as a replacement character while preserving its original native
byte storage.
Consequently, the regular character API is safe to use, and reserialization writes the original bytes unchanged.
Encoders use canonical ``Name: value`` framing and never require a parallel byte-oriented field API.

Incremental HTTP/1.x Codecs
===========================

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
======

Default limits are 8 KiB for a start line, the configured ``HttpHeaderLimits`` separately for headers and trailers, 8
KiB for each chunk-metadata line, 16 MiB for a decoded body, 16 KiB for yielded body chunks, and 1 MiB for each retained
input or output queue.
Limits are checked before attacker-controlled allocation.

Interface
=========

.. doxygenclass:: erbsland::network::HttpRequestHead
    :members:
.. doxygenclass:: erbsland::network::HttpResponseHead
    :members:
