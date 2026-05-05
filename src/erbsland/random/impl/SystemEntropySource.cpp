// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SystemEntropySource.hpp"

namespace erbsland::random::impl {

SystemEntropySource::~SystemEntropySource() = default;

void SystemEntropySource::fillBytes(const std::span<std::byte> destination) {
    _source->fillBytes(destination);
}

}
