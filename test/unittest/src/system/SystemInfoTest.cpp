// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Definitions.hpp>
#include <erbsland/system/CpuArchitecture.hpp>
#include <erbsland/system/OperatingSystem.hpp>
#include <erbsland/system/SystemInfo.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(OperatingSystem CpuArchitecture SystemInfo)
class SystemInfoTest final : public el::UnitTest {
public:
    void testOperatingSystemStrings() {
        REQUIRE_EQUAL(el::system::OperatingSystem{}.toString(), "unknown"_el);
        REQUIRE_EQUAL(el::system::OperatingSystem{el::system::OperatingSystem::Windows}.toString(), "windows"_el);
        REQUIRE_EQUAL(el::system::OperatingSystem{el::system::OperatingSystem::Macos}.toString(), "macos"_el);
        REQUIRE_EQUAL(el::system::OperatingSystem{el::system::OperatingSystem::Linux}.toString(), "linux"_el);
    }

    void testCpuArchitectureStrings() {
        REQUIRE_EQUAL(el::system::CpuArchitecture{}.toString(), "unknown"_el);
        REQUIRE_EQUAL(el::system::CpuArchitecture{el::system::CpuArchitecture::X86}.toString(), "x86"_el);
        REQUIRE_EQUAL(el::system::CpuArchitecture{el::system::CpuArchitecture::X86_64}.toString(), "x86_64"_el);
        REQUIRE_EQUAL(el::system::CpuArchitecture{el::system::CpuArchitecture::Arm32}.toString(), "arm32"_el);
        REQUIRE_EQUAL(el::system::CpuArchitecture{el::system::CpuArchitecture::Arm64}.toString(), "arm64"_el);
    }

    void testNativeSystemInformation() {
#if defined(ERBSLAND_OS_WINDOWS)
        REQUIRE(el::system::info::operatingSystem() == el::system::OperatingSystem::Windows);
#elif defined(ERBSLAND_OS_MACOS)
        REQUIRE(el::system::info::operatingSystem() == el::system::OperatingSystem::Macos);
#elif defined(ERBSLAND_OS_LINUX)
        REQUIRE(el::system::info::operatingSystem() == el::system::OperatingSystem::Linux);
#endif
        REQUIRE(el::system::info::cpuArchitecture() != el::system::CpuArchitecture::Unknown);
        REQUIRE_GREATER_EQUAL(el::system::info::logicalCpuCount(), 1U);
    }
};
