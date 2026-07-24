// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SecureErase.hpp"

#include "impl/SecureErase.hpp"

namespace erbsland::mem {

void secureErase(const ByteSpan span) noexcept {
    impl::secureErase(std::span{reinterpret_cast<std::byte *>(span.data()), span.size()});
}

}
