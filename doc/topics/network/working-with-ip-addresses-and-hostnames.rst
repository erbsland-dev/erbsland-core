.. index::
    single: IP Address
    single: Host Name
    single: Network; Address Values
    single: Network; Endpoints

***************************************
Working with IP Addresses and Hostnames
***************************************

This page introduces the value types used to describe network hosts, endpoints, and address ranges.
You will learn how to choose the right type, parse untrusted or trusted text, format canonical values, work with IPv6
endpoints, and test CIDR network membership.

Parse and Inspect Addresses and Hosts
=====================================

Use :cpp:class:`IpAddress <erbsland::network::IpAddress>` when a value must already be an IPv4 or IPv6 address.
Use
:cpp:class:`HostName <erbsland::network::HostName>` when it must remain a name for the platform resolver. When either
form is valid, :cpp:class:`Host <erbsland::network::Host>` preserves which form was provided and lets you inspect it
with ``isAddress()`` or ``isName()``.

The non-throwing ``fromString()`` factories return ``std::nullopt`` for invalid text and are a good fit for forms,
configuration readers that collect several errors, and other untrusted input.
Use ``fromStringOrThrow()`` for trusted constants or code paths where invalid text shall immediately become a
:cpp:class:`ParseError <erbsland::err::ParseError>`.

.. erbsland-demo::
    :source: network/NetworkValues/ParseAddressesAndHosts.cpp
    :exec: network/network_values --demo ParseAddressesAndHosts
    :source-sha256: adcbfb0b2a17297430633a047a1e22b4109d7cf37fd7ac8dea7ea9945448dca2

.. code-block:: cpp

    /// Use `IpAddress` when input must be numeric, `HostName` when it must be a name, and `Host` when either form is valid.
    /// The non-throwing `fromString()` factories are suitable for user input, while `fromStringOrThrow()` keeps trusted
    /// configuration and constants concise. Formatting always returns a canonical representation.
    void parseAddressesAndHosts() {
        const auto address = el::IpAddress::fromStringOrThrow("2001:0DB8:0:0::42"_el);
        const auto hostName = el::HostName::fromStringOrThrow("stelling.example"_el);
        const auto numericHost = el::Host::fromStringOrThrow("192.0.2.42"_el);
        const auto namedHost = el::Host::fromStringOrThrow("stelling.example"_el);

        el::io::printLine("Canonical address: "_el, address);
        el::io::printLine("Host name: "_el, hostName);
        el::io::printLine("Numeric host contains an address: "_el, numericHost.isAddress());
        el::io::printLine("Named host contains a name: "_el, namedHost.isName());

        const auto invalidHost = el::Host::fromString("not a host"_el);
        el::io::printLine("Invalid user input accepted: "_el, invalidHost.has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Canonical address: 2001:db8::42
    Host name: stelling.example
    Numeric host contains an address: true
    Named host contains a name: true
    Invalid user input accepted: false

.. erbsland-demo-end::

Formatting an address produces canonical text.
IPv6 output uses lowercase hexadecimal digits and compresses zero groups, so compare
:cpp:class:`IpAddress <erbsland::network::IpAddress>` values rather than their original input text.
The value also reports its :cpp:enum:`IpVersion <erbsland::network::IpVersion>` and recognizes any and loopback
addresses.
``anyV4()``, ``anyV6()``, ``loopbackV4()``, and ``loopbackV6()`` provide these common values without parsing.

All address and host types have value semantics.
They can be compared directly and used in standard hash containers; their hash functions are consistent with equality
and canonical address representation.

Internationalized Host Names
============================

``HostName`` accepts Unicode U-labels, decomposed Unicode spelling, ASCII A-labels, and mixed domains.
It validates them with strict IDNA2008, stores lowercase NFC Unicode as the semantic value, and precomputes the ASCII
transport form.
Equivalent spellings therefore compare and hash equally.

.. code-block:: cpp

    const auto first = el::HostName::fromStringOrThrow("www.bücher.example"_el);
    const auto second = el::HostName::fromStringOrThrow("WWW.XN--BCHER-KVA.example"_el);

    assert(first == second);
    assert(first.toString() == "www.bücher.example"_el);
    assert(first.toString(el::HostNameFormat::IdnaAscii) == "www.xn--bcher-kva.example"_el);

Native resolver backends always receive ``IdnaAscii``.
Callbacks, lookup errors, application configuration, and ordinary formatting continue to expose the canonical Unicode
value, so the platform boundary does not leak into the application model.
Terminal dots, empty labels, underscores and service labels, malformed UTF-8, compatibility mappings, and invalid IDNA
characters are rejected.

Build Endpoints without Ambiguity
=================================

An endpoint combines a host with a :cpp:class:`Port <erbsland::network::Port>`.
Use
:cpp:class:`HostEndpoint <erbsland::network::HostEndpoint>` while a name is still allowed and
:cpp:class:`IpEndpoint <erbsland::network::IpEndpoint>` once the address is resolved.

IPv6 endpoint text places the address in brackets, which keeps its colons separate from the port.
A link-local IPv6 address can also require a numeric :cpp:class:`ScopeId <erbsland::network::ScopeId>` identifying its
network interface; the scope follows ``%`` inside the brackets.

.. erbsland-demo::
    :source: network/NetworkValues/BuildEndpoints.cpp
    :exec: network/network_values --demo BuildEndpoints
    :source-sha256: 993e04c06a7964a44d21e1f60afa2a9f278b4c1fc91e8bab8e5eb3e44899da61

.. code-block:: cpp

    /// Pair an `IpAddress` with a `Port` in `IpEndpoint` after resolution, or use `HostEndpoint` while a host name is still
    /// allowed. IPv6 endpoints use brackets, and link-local IPv6 addresses can carry a numeric `ScopeId`.
    void buildEndpoints() {
        const auto service = el::HostEndpoint::fromStringOrThrow("stelling.example:443"_el);
        const auto resolved = el::IpEndpoint::fromStringOrThrow("192.0.2.42:443"_el);
        const auto linkLocal = el::IpEndpoint::fromStringOrThrow("[fe80::42%7]:443"_el);

        el::io::printLine("Named endpoint: "_el, service.toString());
        el::io::printLine("Resolved endpoint: "_el, resolved.toString());
        el::io::printLine("Scoped IPv6 endpoint: "_el, linkLocal.toString());
        el::io::printLine("IPv6 scope identifier: "_el, linkLocal.scopeId().toRawValue());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Named endpoint: stelling.example:443
    Resolved endpoint: 192.0.2.42:443
    Scoped IPv6 endpoint: [fe80::42%7]:443
    IPv6 scope identifier: 7

.. erbsland-demo-end::

A default :cpp:class:`Port <erbsland::network::Port>` has value zero and requests automatic local-port selection.
It is useful when binding a local endpoint, but it is not a valid remote destination.
Prefer the endpoint parsers over manual string splitting, especially for IPv6 and scoped addresses.

Match Networks and Address Ranges
=================================

:cpp:class:`IpNetwork <erbsland::network::IpNetwork>` stores an IPv4 or IPv6 network in CIDR notation. Parsing
normalizes host bits, so ``192.0.2.129/24`` becomes ``192.0.2.0/24``.
Use ``contains()`` to test an address or a complete subnetwork, and use ``firstAddress()`` and ``lastAddress()`` when
you need the inclusive range.

.. erbsland-demo::
    :source: network/NetworkValues/MatchNetworks.cpp
    :exec: network/network_values --demo MatchNetworks
    :source-sha256: d19f514699b3606d16a878cd640746724a14089ac9654dccd6fff61113e9b55a

.. code-block:: cpp

    /// `IpNetwork` represents a normalized IPv4 or IPv6 CIDR range.
    /// Use `contains()` for allowlists, routing decisions, or other address-range checks.
    void matchNetworks() {
        const auto network = el::IpNetwork::fromStringOrThrow("192.0.2.129/24"_el);
        const auto inside = el::IpAddress::fromStringOrThrow("192.0.2.42"_el);
        const auto outside = el::IpAddress::fromStringOrThrow("198.51.100.42"_el);

        el::io::printLine("Normalized network: "_el, network);
        el::io::printLine(inside, " belongs to the network: "_el, network.contains(inside));
        el::io::printLine(outside, " belongs to the network: "_el, network.contains(outside));
        el::io::printLine("Address range: "_el, network.firstAddress(), " - "_el, network.lastAddress());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Normalized network: 192.0.2.0/24
    192.0.2.42 belongs to the network: true
    198.51.100.42 belongs to the network: false
    Address range: 192.0.2.0 - 192.0.2.255

.. erbsland-demo-end::

An IPv4 network accepts prefix lengths from zero through 32; an IPv6 network accepts zero through 128. Address and
network versions must match for containment.
These checks make :cpp:class:`IpNetwork <erbsland::network::IpNetwork>` a good building block for allowlists, routing
rules, and interface selection.

From Host Names to Addresses
============================

A :cpp:class:`HostName <erbsland::network::HostName>` and a name-valued :cpp:class:`Host <erbsland::network::Host>` are
deliberately unresolved.
Keep them in that form while your configuration or request still describes a name.
Resolve them only when an operation needs concrete addresses.

See :doc:`resolving-hosts-asynchronously` for the asynchronous lookup workflow, error handling, cancellation, event-loop
selection, and lifetime management.
