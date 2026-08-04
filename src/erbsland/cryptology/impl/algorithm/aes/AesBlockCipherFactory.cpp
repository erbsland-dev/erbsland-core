// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesBlockCipherFactory.hpp"

#include "HardwareFeatures.hpp"
#include "PortableAesBlockCipher.hpp"

#include "../../../../core/Application.hpp"
#include "../../../configuration/CryptologyConfiguration.hpp"

#if defined(ERBSLAND_AES_ARM_BACKEND)
#include "ArmAesBlockCipher.hpp"
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
#include "X86AesBlockCipher.hpp"
#endif

namespace erbsland::cryptology::impl {

auto createAesBlockCipher(const mem::ConstByteSpan key) -> std::unique_ptr<AesBlockCipher> {
    if (core::application().cryptologyConfiguration().hardwareAccelerationEnabled()) {
#if defined(ERBSLAND_AES_ARM_BACKEND)
        if (hardware_features::hasArmAes()) {
            return std::make_unique<ArmAesBlockCipher>(key);
        }
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
        if (hardware_features::hasX86Aes()) {
            return std::make_unique<X86AesBlockCipher>(key);
        }
#endif
    }
    return createPortableAesBlockCipher(key);
}

auto createPortableAesBlockCipher(const mem::ConstByteSpan key) -> std::unique_ptr<AesBlockCipher> {
    return std::make_unique<PortableAesBlockCipher>(key);
}

}
