..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********************
Network API Overview
********************

The network API combines portable address values with event-based operations.

Address and Host Values
=======================

:doc:`working-with-ip-addresses-and-hostnames` explains how to choose, parse, format, compare, and inspect IP addresses,
host names, endpoints, ports, IPv6 scopes, and CIDR networks.

Resolving Hosts
===============

:doc:`resolving-hosts-asynchronously` explains how to resolve a numeric address or host name without blocking an event
loop, handle every returned address, choose an owner loop, cancel safely, and manage lookup errors and lifetime.

Transports
==========

:doc:`using-udp-sockets` explains how to bind a native UDP socket, exchange repeated datagrams with multiple remotes,
respond to output back-pressure and oversized input, and choose graceful or immediate shutdown.

:doc:`using-tcp-connections` explains outgoing resolution, listeners and admission filters, transferable accepted
sockets, stream back-pressure, and normal or immediate shutdown.

:doc:`using-tls-client-connections` explains authenticated outgoing TLS connections, configuration snapshots,
checkpoints, back-pressure, deadlines, and ``close_notify``.

:doc:`using-tls-server-connections` explains shared accepted-connection and handshake quotas, default and exact-SNI
identities, ClientHello inspection, authenticated stream flow, and graceful shutdown.

HTTP Servers
============

:doc:`using-http-servers` explains how to configure one plaintext or TLS HTTP/1.1 server, register ordered server and
session routes, choose request-body policies, retain requests for delayed work, stream responses, and enable bounded
cookie-backed sessions.

HTTP Clients
============

:doc:`using-http-clients` explains how to prepare and submit HTTP or HTTPS requests through one session, choose
automatic or low-level response handling, stream uploads and downloads under back-pressure, and close the session
cleanly.
