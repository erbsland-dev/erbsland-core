// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GaloisMultiplierFactory.hpp"

#include "HardwareFeatures.hpp"
#include "PortableGaloisMultiplier.hpp"

#include "../../../../core/Application.hpp"
#include "../../../configuration/CryptologyConfiguration.hpp"

#if defined(ERBSLAND_AES_ARM_BACKEND)
#include "ArmGaloisMultiplier.hpp"
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
#include "X86GaloisMultiplier.hpp"
#endif

namespace erbsland::cryptology::impl {

auto createGaloisMultiplier() -> std::unique_ptr<GaloisMultiplier> {
    if (core::application().cryptologyConfiguration().hardwareAccelerationEnabled()) {
#if defined(ERBSLAND_AES_ARM_BACKEND)
        if (hardware_features::hasArmPmull()) {
            return std::make_unique<ArmGaloisMultiplier>();
        }
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
        if (hardware_features::hasX86Pclmul()) {
            return std::make_unique<X86GaloisMultiplier>();
        }
#endif
    }
    return createPortableGaloisMultiplier();
}

auto createPortableGaloisMultiplier() -> std::unique_ptr<GaloisMultiplier> {
    return std::make_unique<PortableGaloisMultiplier>();
}

}
