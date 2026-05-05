// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SystemEntropySource.hpp"

#include "WindowsEntropySource.hpp"

namespace erbsland::random::impl {

SystemEntropySource::SystemEntropySource() : _source{std::make_unique<WindowsEntropySource>()} {
}

}
