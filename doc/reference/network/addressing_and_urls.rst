..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Network Addressing; Reference
    single: URLs; Reference
    single: IP Address
    single: IP Network
    single: Network Endpoint
    single: URL
    single: Network URL

***************************
Network Addressing and URLs
***************************

Network Addressing Types
========================

Address and Network Values
--------------------------

``IpAddress`` stores IPv4 or IPv6 bits and uses canonical lowercase compressed text for IPv6. ``IpNetwork`` stores a
normalized CIDR range and provides containment and boundary operations.
IPv6 zone identifiers are intentionally not part of an address; ``ScopeId`` stores the optional numeric scope on an
endpoint.

Hosts and Endpoints
-------------------

``HostName`` is a strict IDNA2008 value with canonical lowercase NFC Unicode semantics and a canonical ASCII transport
form, while ``Host`` contains either a name or a numeric address.
Default formatting returns Unicode; :cpp:enum:`HostNameFormat::IdnaAscii <erbsland::network::HostNameFormat>` exposes
the A-label/NR-LDH form sent to native resolvers and suitable for SNI or certificate comparison.
``IpEndpoint`` is always resolved.
``HostEndpoint`` can remain unresolved until an asynchronous operation starts.
Bracketed IPv6 endpoint text keeps the numeric scope after ``%`` and before the closing bracket.

Parsing Errors
--------------

Each ``fromStringOrThrow()`` factory is the diagnostic parsing API and raises
:cpp:class:`ParseError <erbsland::err::ParseError>` with the specific invalid component or syntax rule. The matching
``fromString()`` factory catches that parse error and returns ``std::nullopt``, making it suitable for tolerant input
tests without losing detailed diagnostics where they are needed.

See :doc:`/topics/network/working-with-ip-addresses-and-hostnames` for parsing, formatting, endpoint construction, and
CIDR range examples.

URLs
====

Network URL Values
------------------

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
-------------------

``resolved()`` and ``resolvedOrThrow()`` apply RFC 3986 reference resolution to hierarchical URLs.
They support absolute, scheme-relative, absolute-path, relative-path, query-only, fragment-only, and empty references.
Resolution removes dot segments from the resulting path; ordinary absolute parsing continues to retain them.

HTTP, HTTPS, FTP, FTPS, file, and authority-bearing custom URLs can act as bases.
Mail addresses and authority-free custom URLs are opaque and cannot resolve relative references.

Safety and Display
------------------

``fromString()`` returns the invalid ``Url{}`` placeholder for malformed input; ``fromStringOrThrow()`` reports a
detailed :cpp:class:`ParseError <erbsland::err::ParseError>`.
``UrlParseOptions`` defaults to a 16 KiB input limit.

``UrlFormatOptions`` selects IDNA ASCII or Unicode host names and controls default-port and fragment output.
Passwords are redacted by default and can be explicitly revealed.
For an opaque custom authority, the default redaction replaces the complete authority.

Interface
=========

.. doxygenclass:: erbsland::network::Host
    :members:
.. doxygenclass:: erbsland::network::HostEndpoint
    :members:
.. doxygenclass:: erbsland::network::HostName
    :members:
.. doxygenenum:: erbsland::network::HostNameFormat
.. doxygenclass:: erbsland::network::IpAddress
    :members:
.. doxygenclass:: erbsland::network::IpEndpoint
    :members:
.. doxygenclass:: erbsland::network::IpNetwork
    :members:
.. doxygenenum:: erbsland::network::IpVersion
.. doxygenclass:: erbsland::network::Port
    :members:
.. doxygenclass:: erbsland::network::ScopeId
    :members:
.. doxygenclass:: erbsland::network::Url
    :members:
.. doxygenclass:: erbsland::network::UrlFormatOptions
    :members:
.. doxygenclass:: erbsland::network::UrlParseOptions
    :members:
.. doxygenenum:: erbsland::network::UrlScheme
