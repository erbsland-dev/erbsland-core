// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationProtectedDataAccess.hpp"

#include "../../../core/Application.hpp"
#include "../../configuration/CryptologyConfiguration.hpp"

namespace erbsland::cryptology::impl {

auto ApplicationProtectedDataAccess::protect(const mem::ConstByteSpan plaintext, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
    return core::application().cryptologyConfiguration().protectData(plaintext, plaintextLength);
}

auto ApplicationProtectedDataAccess::unprotect(
    const mem::ConstByteSpan envelope, const unit::ByteLength plaintextLength) -> mem::ByteBlock {
    return core::application().cryptologyConfiguration().unprotectData(envelope, plaintextLength);
}

}
