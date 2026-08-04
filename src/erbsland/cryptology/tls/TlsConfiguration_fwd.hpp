// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::cryptology {

class TlsConfiguration;
using TlsConfigurationPtr = std::shared_ptr<TlsConfiguration>;
using TlsConfigurationConstPtr = std::shared_ptr<const TlsConfiguration>;

class TlsConfigurationResolution;

}
