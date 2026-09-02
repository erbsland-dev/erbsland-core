// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/Size.hpp>
#include <erbsland/block/StdFormat.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/ProcessInfo.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/SystemInfo.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <ratio>
#include <string>
#include <type_traits>

using namespace el::text::literals;
using namespace el::time::literals;

TESTED_TARGETS(Host Literals MakeOneNamespace Network ProcessInfo StdFormat SystemInfo)
class MergedIncludeHeadersTest final : public el::UnitTest {
private:
    struct MergedFormatUnitTag {};

public:
    void testLiterals() {
        REQUIRE_EQUAL(el::text::String{"merged"_el}, el::text::String{"merged"});
        REQUIRE_EQUAL(750_ms, el::time::Milliseconds{750});
    }

    void testStdFormat() {
        using SampleAmount = el::unit::IntegerAmount<MergedFormatUnitTag, std::ratio<1>>;

        const auto blockSize = el::block::Size{8, 5};
        const auto text = el::text::String{"merged"_el};
        const auto date = el::time::Date::fromYearMonthDay(2026, 7, 28);
        const auto amount = SampleAmount{42};
        const auto formattedBlockSize = std::format("{}", blockSize);
        const auto formattedText = std::format("{}", text);
        const auto formattedDate = std::format("{}", date);
        const auto formattedAmount = std::format("{}", amount);

        REQUIRE_EQUAL(formattedBlockSize, std::string{"8x5"});
        REQUIRE_EQUAL(formattedText, std::string{"merged"});
        REQUIRE_EQUAL(formattedDate, std::string{"2026-07-28"});
        REQUIRE_EQUAL(formattedAmount, std::string{"42"});
    }

    void testNetworkNamespaceIsFlattened() {
        REQUIRE((std::is_same_v<el::Host, el::network::Host>));
        REQUIRE((std::is_same_v<el::Network, el::network::Network>));
    }

    void testSystemNamespaceIsFlattened() {
        REQUIRE((std::is_same_v<el::ProcessInfo, el::system::ProcessInfo>));
        REQUIRE((std::is_same_v<el::OperatingSystem, el::system::OperatingSystem>));
        REQUIRE(el::sys_info::operatingSystem() != el::OperatingSystem::Unknown);
        REQUIRE_GREATER_EQUAL(el::sys_info::logicalCpuCount(), 1U);
    }
};
