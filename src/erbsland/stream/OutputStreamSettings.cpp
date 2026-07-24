// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OutputStreamSettings.hpp"

#include "impl/StreamBufferSizes.hpp"

namespace erbsland::stream {

auto OutputStreamSettings::backBufferLimit() const noexcept -> unit::ByteLength {
    return _backBufferLimit.value_or(impl::streamBufferSizes(_buffering).outputBackLimit);
}

}
