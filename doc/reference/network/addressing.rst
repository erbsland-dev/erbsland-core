.. index::
    single: IP Address
    single: IP Network
    single: Network Endpoint

************************
Network Addressing Types
************************

Address and Network Values
==========================

``IpAddress`` stores IPv4 or IPv6 bits and uses canonical lowercase compressed text for IPv6. ``IpNetwork`` stores a
normalized CIDR range and provides containment and boundary operations.
IPv6 zone identifiers are intentionally not part of an address; ``ScopeId`` stores the optional numeric scope on an
endpoint.

Hosts and Endpoints
===================

``HostName`` is a strict IDNA2008 value with canonical lowercase NFC Unicode semantics and a canonical ASCII transport
form, while ``Host`` contains either a name or a numeric address.
Default formatting returns Unicode; :cpp:enum:`HostNameFormat::IdnaAscii <erbsland::network::HostNameFormat>` exposes
the A-label/NR-LDH form sent to native resolvers and suitable for SNI or certificate comparison.
``IpEndpoint`` is always resolved.
``HostEndpoint`` can remain unresolved until an asynchronous operation starts.
Bracketed IPv6 endpoint text keeps the numeric scope after ``%`` and before the closing bracket.

Parsing Errors
==============

Each ``fromStringOrThrow()`` factory is the diagnostic parsing API and raises
:cpp:class:`ParseError <erbsland::err::ParseError>` with the specific invalid component or syntax rule. The matching
``fromString()`` factory catches that parse error and returns ``std::nullopt``, making it suitable for tolerant input
tests without losing detailed diagnostics where they are needed.

See :doc:`/topics/network/working-with-ip-addresses-and-hostnames` for parsing, formatting, endpoint construction, and
CIDR range examples.

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
