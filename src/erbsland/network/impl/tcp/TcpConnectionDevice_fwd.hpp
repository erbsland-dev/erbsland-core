// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network::impl {

class TcpConnectionDevice;
using TcpConnectionDevicePtr = std::shared_ptr<TcpConnectionDevice>;

}
