// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network::impl {

class PosixTcpConnectionDevice;
using PosixTcpConnectionDevicePtr = std::shared_ptr<PosixTcpConnectionDevice>;

}
