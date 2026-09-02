// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/engine/CaptureGroupManagerBase.hpp>
#include <erbsland/re/impl/engine/CaptureGroupManagerWithAtomic.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(CaptureGroupManager)
TAGS(Matching)
class CaptureGroupManagerTest final : public el::UnitTest {
    using CaptureGroupManager = el::re::impl::CaptureGroupManager;
    template <std::size_t tGroupCount>
    using CaptureGroupManagerBase = el::re::impl::CaptureGroupManagerBase<tGroupCount>;
    template <std::size_t tGroupCount>
    using CaptureGroupManagerWithAtomic = el::re::impl::CaptureGroupManagerWithAtomic<tGroupCount>;

public:
    void testRegularSizeClasses() {
        REQUIRE(isManager<CaptureGroupManagerBase<1>>(1, false));
        REQUIRE(isManager<CaptureGroupManagerBase<2>>(2, false));
        REQUIRE(isManager<CaptureGroupManagerBase<3>>(3, false));
        REQUIRE(isManager<CaptureGroupManagerBase<5>>(4, false));
        REQUIRE(isManager<CaptureGroupManagerBase<9>>(6, false));
        REQUIRE(isManager<CaptureGroupManagerBase<17>>(10, false));
        REQUIRE(isManager<CaptureGroupManagerBase<33>>(18, false));
        REQUIRE(isManager<CaptureGroupManagerBase<65>>(34, false));
    }

    void testAtomicSizeClasses() {
        REQUIRE(isManager<CaptureGroupManagerWithAtomic<1>>(1, true));
        REQUIRE(isManager<CaptureGroupManagerWithAtomic<3>>(2, true));
        REQUIRE(isManager<CaptureGroupManagerWithAtomic<7>>(4, true));
        REQUIRE(isManager<CaptureGroupManagerWithAtomic<15>>(8, true));
        REQUIRE(isManager<CaptureGroupManagerWithAtomic<31>>(16, true));
        REQUIRE(isManager<CaptureGroupManagerWithAtomic<63>>(32, true));
    }

private:
    template <typename Expected>
    [[nodiscard]] static auto isManager(const std::size_t groupCount, const bool hasAtomicGroups) -> bool {
        const auto manager = CaptureGroupManager::create(groupCount, hasAtomicGroups);
        return dynamic_cast<Expected *>(manager.get()) != nullptr;
    }
};
