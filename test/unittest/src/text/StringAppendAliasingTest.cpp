// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/AnyStringBuilder.hpp>
#include <erbsland/AnyStringEditor.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/U16String.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32String.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/U8String.hpp>
#include <erbsland/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <type_traits>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(
    StringEditor U16StringEditor U32StringEditor AnyStringBuilder U8StringAppendTools U16StringAppendTools
        U32StringAppendTools)
class StringAppendAliasingTest final : public el::UnitTest {
private:
    template <typename T>
    [[nodiscard]] static auto sample() -> T {
        if constexpr (std::is_same_v<T, U8String>) {
            return u8"Aé😀\0Z"_el;
        } else if constexpr (std::is_same_v<T, U16String>) {
            return u"Aé😀\0Z"_el;
        } else {
            return U"Aé😀\0Z"_el;
        }
    }

    template <typename T>
    [[nodiscard]] static auto snapshot(const AnyStringBuilder &builder) -> T {
        if constexpr (std::is_same_v<T, U8String>) {
            return builder.toU8String();
        } else if constexpr (std::is_same_v<T, U16String>) {
            return builder.toU16String();
        } else {
            return builder.toU32String();
        }
    }

    template <typename tString, typename tEditor>
    void verifySelfAppend(const bool reserveCapacity) {
        auto editor = tEditor{sample<tString>()};
        if (reserveCapacity) {
            editor.reserve(decltype(editor.length()){256U});
        } else {
            editor.shrinkToFit();
        }
        // The implicit read-only argument must retain the old allocation before the editor detaches.
        editor.append(editor);
        REQUIRE_EQUAL(StringConverter{editor}.toU8String(), u8"Aé😀\0ZAé😀\0Z"_el);

        editor.append(editor, ItemCount{2U});
        REQUIRE_EQUAL(StringConverter{editor}.toU8String(), u8"Aé😀\0ZAé😀\0ZAé😀\0ZAé😀\0ZAé😀\0ZAé😀\0Z"_el);
    }

    template <typename tString, typename tEditor>
    void verifySharedSliceAppend() {
        auto editor = tEditor{sample<tString>()};
        editor.reserve(decltype(editor.length()){256U});
        const auto source = tString{editor}.slice(CpRange{CpIndex{1U}, CpCount{2U}});
        const auto sourceStorage = source.storageId();
        const auto destinationStorage = editor.storageId();
        editor.append(source, ItemCount{2U});
        REQUIRE_EQUAL(StringConverter{editor}.toU8String(), u8"Aé😀\0Zé😀é😀"_el);
        REQUIRE_EQUAL(StringConverter{source}.toU8String(), u8"é😀"_el);
        REQUIRE_EQUAL(source.storageId(), sourceStorage);
        REQUIRE_NOT_EQUAL(editor.storageId(), destinationStorage);

        // Destroy the original editor before appending to a uniquely owned slice.
        auto slice = tEditor{sample<tString>()}.slice(CpRange{CpIndex{1U}, CpCount{2U}});
        slice.append(slice, ItemCount{2U});
        REQUIRE_EQUAL(StringConverter{slice}.toU8String(), u8"é😀é😀é😀"_el);
    }

    template <typename T>
    void verifyBuilderAppends() {
        for (const auto reserveCapacity : {false, true}) {
            auto builder = AnyStringBuilder::basedOn(sample<T>());
            if (reserveCapacity) {
                builder = AnyStringBuilder::basedOn(sample<T>(), decltype(sample<T>().length()){256U});
            }
            const auto source = snapshot<T>(builder);
            builder.append(source);
            REQUIRE_EQUAL(builder.toU8String(), u8"Aé😀\0ZAé😀\0Z"_el);
            REQUIRE_EQUAL(source, sample<T>());
            REQUIRE_NOT_EQUAL(snapshot<T>(builder).storageId(), source.storageId());
            REQUIRE_EQUAL(builder.length(), CpCount{10U});

            const auto slice = snapshot<T>(builder).slice(CpRange{CpIndex{1U}, CpCount{2U}});
            builder.append(slice, ItemCount{2U});
            REQUIRE_EQUAL(builder.toU8String(), u8"Aé😀\0ZAé😀\0Zé😀é😀"_el);
            REQUIRE_EQUAL(StringConverter{slice}.toU8String(), u8"é😀"_el);
            REQUIRE_EQUAL(builder.length(), CpCount{14U});
        }

        auto builder = AnyStringBuilder::basedOn(sample<T>());
        builder.append(snapshot<T>(builder), ItemCount{2U});
        REQUIRE_EQUAL(builder.toU8String(), u8"Aé😀\0ZAé😀\0ZAé😀\0Z"_el);
        REQUIRE_EQUAL(builder.length(), CpCount{15U});

        auto copy = builder;
        builder.appendAny(copy.toAnyStringEditor());
        REQUIRE_EQUAL(copy.toU8String(), u8"Aé😀\0ZAé😀\0ZAé😀\0Z"_el);
        REQUIRE_EQUAL(builder.toU8String(), u8"Aé😀\0ZAé😀\0ZAé😀\0ZAé😀\0ZAé😀\0ZAé😀\0Z"_el);
        REQUIRE_EQUAL(builder.length(), CpCount{30U});
        REQUIRE_EQUAL(copy.length(), CpCount{15U});
    }

public:
    void testUtf8EditorSelfAppend() {
        WITH_CONTEXT((verifySelfAppend<U8String, U8StringEditor>(false)));
        WITH_CONTEXT((verifySelfAppend<U8String, U8StringEditor>(true)));
        WITH_CONTEXT((verifySharedSliceAppend<U8String, U8StringEditor>()));
    }

    void testUtf16EditorSelfAppend() {
        WITH_CONTEXT((verifySelfAppend<U16String, U16StringEditor>(false)));
        WITH_CONTEXT((verifySelfAppend<U16String, U16StringEditor>(true)));
        WITH_CONTEXT((verifySharedSliceAppend<U16String, U16StringEditor>()));
    }

    void testUtf32EditorSelfAppend() {
        WITH_CONTEXT((verifySelfAppend<U32String, U32StringEditor>(false)));
        WITH_CONTEXT((verifySelfAppend<U32String, U32StringEditor>(true)));
        WITH_CONTEXT((verifySharedSliceAppend<U32String, U32StringEditor>()));
    }

    void testUtf8BuilderSharedSource() { WITH_CONTEXT(verifyBuilderAppends<U8String>()); }

    void testUtf16BuilderSharedSource() { WITH_CONTEXT(verifyBuilderAppends<U16String>()); }

    void testUtf32BuilderSharedSource() { WITH_CONTEXT(verifyBuilderAppends<U32String>()); }
};
