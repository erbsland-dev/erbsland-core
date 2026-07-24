// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IpAddress.hpp"
#include "Network_fwd.hpp"
#include "NetworkError.hpp"
#include "UdpDatagram.hpp"

#include "../mem/ByteBlock.hpp"
#include "../util/List.hpp"

#include <functional>

namespace erbsland::network {

/// A callback for a network state transition without associated data.
using NetworkEventFn = std::function<void()>;
/// A callback receiving an asynchronous network error.
using NetworkErrorFn = std::function<void(const NetworkError &)>;
/// A callback receiving the resolved addresses for a host lookup.
using HostResolvedFn = std::function<void(const util::List<IpAddress> &)>;
/// A callback receiving a pending incoming TCP connection request.
using TcpConnectionRequestFn = std::function<void(TcpConnectionRequestPtr)>;
/// A callback receiving a newly connected TCP stream.
using TcpConnectionFn = std::function<void(TcpConnectionPtr)>;
/// A callback receiving an owned block of stream or fixed-peer data.
using NetworkDataFn = std::function<void(mem::ByteBlock)>;
/// A callback receiving an owned UDP datagram.
using UdpDatagramFn = std::function<void(UdpDatagram)>;

}
