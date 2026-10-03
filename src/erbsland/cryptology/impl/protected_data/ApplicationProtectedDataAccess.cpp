// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationProtectedDataAccess.hpp"

#include "../../../core/impl/application_data/ApplicationCryptologyData.hpp"
#include "../../../core/impl/ApplicationDataAccess.hpp"
#include "../../configuration/CryptologyConfiguration.hpp"

namespace erbsland::cryptology::impl {

using namespace mem;

auto ApplicationProtectedDataAccess::protect(const ConstByteSpan plaintext, const unit::ByteCount plaintextLength)
    -> ByteBlock {
    const auto cryptologyData = core::impl::applicationData()->cryptology().get();
    return cryptologyData->configuration().protectData(plaintext, plaintextLength);
}

auto ApplicationProtectedDataAccess::unprotect(const ConstByteSpan envelope, const unit::ByteCount plaintextLength)
    -> ByteBlock {
    const auto cryptologyData = core::impl::applicationData()->cryptology().get();
    return cryptologyData->configuration().unprotectData(envelope, plaintextLength);
}

}
