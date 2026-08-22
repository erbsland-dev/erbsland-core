// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticContent.hpp"

namespace erbsland::network {

auto HttpStaticContent::retainedMemoryLength() const noexcept -> unit::ByteLength {
    return length();
}

}
