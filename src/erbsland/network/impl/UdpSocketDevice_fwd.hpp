// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network::impl {

class UdpSocketDevice;

/// An exclusively owned native UDP socket device.
using UdpSocketDevicePtr = std::unique_ptr<UdpSocketDevice>;

}
