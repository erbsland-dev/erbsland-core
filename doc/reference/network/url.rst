.. index::
    single: URL
    single: Network URL

****
URLs
****

Network URL Values
==================

``Url`` represents absolute URLs intended for network operations.
HTTP, HTTPS, FTP, and FTPS authorities are parsed into ``HostEndpoint`` values with their default ports.
File and mailto URLs use scheme-specific rules, while unknown schemes retain a custom authority and parse its endpoint
only when possible.

Parsing strictly decodes percent-encoded UTF-8 and NFC-normalizes path, query, fragment, and user-info values.
Dot segments remain unchanged.
``authorityText()`` retains the source authority, but ``toString()`` creates canonical transport text and therefore does
not preserve the original encoding spelling.

``hasQuery()`` and ``hasFragment()`` distinguish an absent delimiter from an explicitly empty query or fragment.
Canonical output retains explicitly empty delimiters.

Relative References
===================

``resolved()`` and ``resolvedOrThrow()`` apply RFC 3986 reference resolution to hierarchical URLs.
They support absolute, scheme-relative, absolute-path, relative-path, query-only, fragment-only, and empty references.
Resolution removes dot segments from the resulting path; ordinary absolute parsing continues to retain them.

HTTP, HTTPS, FTP, FTPS, file, and authority-bearing custom URLs can act as bases.
Mail addresses and authority-free custom URLs are opaque and cannot resolve relative references.

Safety and Display
==================

``fromString()`` returns the invalid ``Url{}`` placeholder for malformed input; ``fromStringOrThrow()`` reports a
detailed :cpp:class:`ParseError <erbsland::err::ParseError>`.
``UrlParseOptions`` defaults to a 16 KiB input limit.

``UrlFormatOptions`` selects IDNA ASCII or Unicode host names and controls default-port and fragment output.
Passwords are redacted by default and can be explicitly revealed.
For an opaque custom authority, the default redaction replaces the complete authority.

Interface
=========

.. doxygenclass:: erbsland::network::Url
    :members:
.. doxygenclass:: erbsland::network::UrlFormatOptions
    :members:
.. doxygenclass:: erbsland::network::UrlParseOptions
    :members:
.. doxygenenum:: erbsland::network::UrlScheme
