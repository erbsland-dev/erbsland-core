// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ChaCha20BackendFactory.hpp"

#include "PortableChaCha20Backend.hpp"

#include "../../../../core/Application.hpp"
#include "../../../configuration/CryptologyConfiguration.hpp"

#if defined(ERBSLAND_CHACHA20_ARM_BACKEND)
#include "ArmChaCha20Backend.hpp"
#endif
#if defined(ERBSLAND_CHACHA20_X86_BACKEND)
#include "X86ChaCha20Backend.hpp"
#endif

namespace erbsland::cryptology::impl {

auto createChaCha20Backend(const mem::ConstByteSpan key, const mem::ConstByteSpan nonce)
    -> std::unique_ptr<ChaCha20Backend> {
    if (core::application().cryptologyConfiguration().hardwareAccelerationEnabled()) {
#if defined(ERBSLAND_CHACHA20_ARM_BACKEND)
        return std::make_unique<ArmChaCha20Backend>(key, nonce);
#elif defined(ERBSLAND_CHACHA20_X86_BACKEND)
        return std::make_unique<X86ChaCha20Backend>(key, nonce);
#endif
    }
    return createPortableChaCha20Backend(key, nonce);
}

auto createPortableChaCha20Backend(const mem::ConstByteSpan key, const mem::ConstByteSpan nonce)
    -> std::unique_ptr<ChaCha20Backend> {
    return std::make_unique<PortableChaCha20Backend>(key, nonce);
}

}
