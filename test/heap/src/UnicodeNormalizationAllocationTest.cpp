// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AllocationTestScope.hpp"

#include <erbsland/text/NormalizationForm.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text;

TESTED_TARGETS(U8String U16String U32String)
class UnicodeNormalizationAllocationTest final : public el::UnitTest {
public:
    void testUnchangedInputDoesNotAllocate() {
        auto source = U32StringEditor{};
        source.append(Char{U'A'}, el::unit::CpLength{1024});
        const auto u32 = U32String{source};
        const auto u16 = StringConverter{u32}.toU16String();
        const auto u8 = StringConverter{u32}.toU8String();

        auto u8Scope = erbsland::test::AllocationTestScope{};
        const auto normalizedU8 = u8.normalized(NormalizationForm::Nfkc);
        const auto u8Allocations = u8Scope.finish();
        REQUIRE_EQUAL(u8Allocations, 0U);
        REQUIRE_EQUAL(normalizedU8.storageId(), u8.storageId());

        auto u16Scope = erbsland::test::AllocationTestScope{};
        const auto normalizedU16 = u16.normalized(NormalizationForm::Nfkc);
        const auto u16Allocations = u16Scope.finish();
        REQUIRE_EQUAL(u16Allocations, 0U);
        REQUIRE_EQUAL(normalizedU16.storageId(), u16.storageId());

        auto u32Scope = erbsland::test::AllocationTestScope{};
        const auto normalizedU32 = u32.normalized(NormalizationForm::Nfkc);
        const auto u32Allocations = u32Scope.finish();
        REQUIRE_EQUAL(u32Allocations, 0U);
        REQUIRE_EQUAL(normalizedU32.storageId(), u32.storageId());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testLargeUnchangedInputDoesNotAllocate() {
        auto editor = U8StringEditor{};
        editor.append(Char{U'A'}, el::unit::CpLength{1'000'000});
        const auto source = U8String{editor};

        auto scope = erbsland::test::AllocationTestScope{};
        const auto normalized = source.normalized(NormalizationForm::Nfkc);
        const auto allocations = scope.finish();
        REQUIRE_EQUAL(allocations, 0U);
        REQUIRE_EQUAL(normalized.storageId(), source.storageId());
    }
};
