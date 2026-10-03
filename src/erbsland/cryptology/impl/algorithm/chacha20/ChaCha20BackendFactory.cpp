// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ChaCha20BackendFactory.hpp"

#include "PortableChaCha20Backend.hpp"

#include "../../../../core/impl/application_data/ApplicationCryptologyData.hpp"
#include "../../../../core/impl/ApplicationDataAccess.hpp"
#include "../../../configuration/CryptologyConfiguration.hpp"

#if defined(ERBSLAND_CHACHA20_ARM_BACKEND)
#include "ArmChaCha20Backend.hpp"
#endif
#if defined(ERBSLAND_CHACHA20_X86_BACKEND)
#include "X86ChaCha20Backend.hpp"
#endif

namespace erbsland::cryptology::impl {

using namespace mem;

auto createChaCha20Backend(const ConstByteSpan key, const ConstByteSpan nonce) -> std::unique_ptr<ChaCha20Backend> {
    const auto cryptologyData = core::impl::applicationData()->cryptology().get();
    if (cryptologyData->configuration().hardwareAccelerationEnabled()) {
#if defined(ERBSLAND_CHACHA20_ARM_BACKEND)
        return std::make_unique<ArmChaCha20Backend>(key, nonce);
#elif defined(ERBSLAND_CHACHA20_X86_BACKEND)
        return std::make_unique<X86ChaCha20Backend>(key, nonce);
#endif
    }
    return createPortableChaCha20Backend(key, nonce);
}

auto createPortableChaCha20Backend(const ConstByteSpan key, const ConstByteSpan nonce)
    -> std::unique_ptr<ChaCha20Backend> {
    return std::make_unique<PortableChaCha20Backend>(key, nonce);
}

}
