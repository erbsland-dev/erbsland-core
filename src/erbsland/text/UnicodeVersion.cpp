// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeVersion.hpp"

#include "impl/UnicodeData.hpp"

namespace erbsland::text {

auto ucdVersion() noexcept -> unit::Version {
    return impl::unicodeDataVersion();
}

}
