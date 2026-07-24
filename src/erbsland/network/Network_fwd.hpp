// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::network {

class HostLookup;
class HostLookupEvents;
class Network;
class TcpConnection;
class TcpConnectionAttempt;
class TcpConnectionAttemptEvents;
class TcpConnectionEvents;
class TcpConnectionRequest;
class TcpListener;
class TcpListenerEvents;
class UdpPeer;
class UdpPeerEvents;
class UdpSocket;
class UdpSocketEvents;

/// A shared host lookup.
using HostLookupPtr = std::shared_ptr<HostLookup>;
/// A shared host lookup callback editor.
using HostLookupEventsPtr = std::shared_ptr<HostLookupEvents>;
/// A shared connected TCP stream.
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
/// A shared outgoing TCP connection attempt.
using TcpConnectionAttemptPtr = std::shared_ptr<TcpConnectionAttempt>;
/// A shared TCP connection-attempt callback editor.
using TcpConnectionAttemptEventsPtr = std::shared_ptr<TcpConnectionAttemptEvents>;
/// A shared TCP connection callback editor.
using TcpConnectionEventsPtr = std::shared_ptr<TcpConnectionEvents>;
/// A shared pending incoming TCP connection request.
using TcpConnectionRequestPtr = std::shared_ptr<TcpConnectionRequest>;
/// A shared TCP listener.
using TcpListenerPtr = std::shared_ptr<TcpListener>;
/// A shared TCP listener callback editor.
using TcpListenerEventsPtr = std::shared_ptr<TcpListenerEvents>;
/// A shared connected UDP peer.
using UdpPeerPtr = std::shared_ptr<UdpPeer>;
/// A shared UDP peer callback editor.
using UdpPeerEventsPtr = std::shared_ptr<UdpPeerEvents>;
/// A shared unconnected UDP socket.
using UdpSocketPtr = std::shared_ptr<UdpSocket>;
/// A shared UDP socket callback editor.
using UdpSocketEventsPtr = std::shared_ptr<UdpSocketEvents>;

}
