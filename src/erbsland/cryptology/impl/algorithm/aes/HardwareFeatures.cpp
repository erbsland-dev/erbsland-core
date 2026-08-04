// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HardwareFeatures.hpp"

#include "../../../../core/Definitions.hpp"

#if defined(ERBSLAND_AES_X86_BACKEND)
#include <array>
#if defined(ERBSLAND_COMPILER_MSVC)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

#if defined(ERBSLAND_AES_ARM_BACKEND) && defined(ERBSLAND_OS_LINUX)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif

#if defined(ERBSLAND_AES_ARM_BACKEND) && defined(ERBSLAND_OS_WINDOWS)
#include "../../../../core/impl/WindowsApi.hpp"
#endif

namespace erbsland::cryptology::impl::hardware_features {

auto hasX86Aes() noexcept -> bool {
#if defined(ERBSLAND_AES_X86_BACKEND)
#if defined(ERBSLAND_COMPILER_MSVC)
    auto registers = std::array<int, 4>{};
    __cpuid(registers.data(), 1);
    return (registers[2] & (1 << 25)) != 0;
#else
    auto eax = 0U;
    auto ebx = 0U;
    auto ecx = 0U;
    auto edx = 0U;
    return __get_cpuid(1U, &eax, &ebx, &ecx, &edx) != 0 && (ecx & bit_AES) != 0U;
#endif
#else
    return false;
#endif
}

auto hasX86Pclmul() noexcept -> bool {
#if defined(ERBSLAND_AES_X86_BACKEND)
#if defined(ERBSLAND_COMPILER_MSVC)
    auto registers = std::array<int, 4>{};
    __cpuid(registers.data(), 1);
    return (registers[2] & (1 << 1)) != 0;
#else
    auto eax = 0U;
    auto ebx = 0U;
    auto ecx = 0U;
    auto edx = 0U;
    return __get_cpuid(1U, &eax, &ebx, &ecx, &edx) != 0 && (ecx & bit_PCLMUL) != 0U;
#endif
#else
    return false;
#endif
}

auto hasArmAes() noexcept -> bool {
#if defined(ERBSLAND_AES_ARM_BACKEND) && defined(ERBSLAND_OS_MACOS)
    // Every Apple Silicon target supported by this build exposes FEAT_AES and FEAT_PMULL.
    return true;
#elif defined(ERBSLAND_AES_ARM_BACKEND) && defined(ERBSLAND_OS_LINUX)
    return (getauxval(AT_HWCAP) & HWCAP_AES) != 0U;
#elif defined(ERBSLAND_AES_ARM_BACKEND) && defined(ERBSLAND_OS_WINDOWS)
    return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) != FALSE;
#else
    return false;
#endif
}

auto hasArmPmull() noexcept -> bool {
#if defined(ERBSLAND_AES_ARM_BACKEND) && defined(ERBSLAND_OS_MACOS)
    return true;
#elif defined(ERBSLAND_AES_ARM_BACKEND) && defined(ERBSLAND_OS_LINUX)
    return (getauxval(AT_HWCAP) & HWCAP_PMULL) != 0U;
#elif defined(ERBSLAND_AES_ARM_BACKEND) && defined(ERBSLAND_OS_WINDOWS)
    return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) != FALSE;
#else
    return false;
#endif
}

}
