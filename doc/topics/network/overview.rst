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
