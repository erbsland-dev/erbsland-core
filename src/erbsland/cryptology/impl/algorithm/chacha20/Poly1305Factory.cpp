// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Poly1305Factory.hpp"

#include "PortablePoly1305.hpp"

#include "../../../../core/Application.hpp"
#include "../../../configuration/CryptologyConfiguration.hpp"

#if defined(ERBSLAND_CHACHA20_ARM_BACKEND)
#include "ArmPoly1305.hpp"
#endif
#if defined(ERBSLAND_CHACHA20_X86_BACKEND)
#include "X86Poly1305.hpp"
#endif

namespace erbsland::cryptology::impl {

auto createPoly1305(const mem::ConstByteSpan key) -> std::unique_ptr<Poly1305> {
    if (core::application().cryptologyConfiguration().hardwareAccelerationEnabled()) {
#if defined(ERBSLAND_CHACHA20_ARM_BACKEND)
        return std::make_unique<ArmPoly1305>(key);
#elif defined(ERBSLAND_CHACHA20_X86_BACKEND)
        return std::make_unique<X86Poly1305>(key);
#endif
    }
    return createPortablePoly1305(key);
}

auto createPortablePoly1305(const mem::ConstByteSpan key) -> std::unique_ptr<Poly1305> {
    return std::make_unique<PortablePoly1305>(key);
}

}
