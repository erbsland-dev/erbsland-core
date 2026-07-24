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

``HostName`` is a validated, opaque name for the platform resolver, while ``Host`` contains either a name or a numeric
address.
``IpEndpoint`` is always resolved.
``HostEndpoint`` can remain unresolved until an asynchronous operation starts.
Bracketed IPv6 endpoint text keeps the numeric scope after ``%`` and before the closing bracket.

Interface
=========

.. doxygenclass:: erbsland::network::Host
    :members:
.. doxygenclass:: erbsland::network::HostEndpoint
    :members:
.. doxygenclass:: erbsland::network::HostName
    :members:
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
