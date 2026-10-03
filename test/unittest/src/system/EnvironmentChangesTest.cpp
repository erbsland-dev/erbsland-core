// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/EnvironmentChanges.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/SubprocessOptions.hpp>
#include <erbsland/system/impl/environment/EnvironmentChangesProcessor.hpp>
#include <erbsland/system/impl/environment/EnvironmentVariableBackend.hpp>
#include <erbsland/text/impl/UnsafeU16StringAccess.hpp>
#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>

using namespace el::text::literals;

TESTED_TARGETS(
    EnvironmentChanges EnvironmentInherit EnvironmentVariableBackend EnvironmentChangesProcessor SubprocessOptions)
class EnvironmentChangesTest final : public el::UnitTest {
private:
    static constexpr auto cAVariable = "A"_el;
    static constexpr auto cPathVariable = "PATH"_el;

    using Changes = el::system::EnvironmentChanges;
    using Inherit = el::system::EnvironmentInherit;
    using Backend = el::system::impl::EnvironmentVariableBackend;
    using Processor = el::system::impl::EnvironmentChangesProcessor;
    using Map = el::text::StringMap<el::text::String>;
    using Path = el::path::Path;

    static auto pathText(const el::String &value) -> el::String {
#ifdef EL_OS_WINDOWS
        return value.replacedAll(el::CharSet{U':'}, el::Char{U';'}).replacedAll(el::CharSet{U'/'}, el::Char{U'\\'});
#else
        return value;
#endif
    }

    void requirePath(const Changes &changes, const el::String &base, const el::String &expected) {
        const auto result = changes.apply(Map{{cPathVariable, pathText(base)}});
        REQUIRE(result.contains(cPathVariable));
        REQUIRE_EQUAL(result.get(cPathVariable).value(), pathText(expected));
    }

public:
    void testInheritanceAndSourceIndependence() {
        const auto base = Map{{"FIRST"_el, "one"_el}, {"EMPTY"_el, ""_el}, {cPathVariable, "a:a:"_el}};
        auto changes = Changes{};
        REQUIRE(changes.isUnchangedInheritance());
        REQUIRE(std::ranges::equal(changes.apply(base), base));
        changes.setInherit(el::text::StringList{"EMPTY"_el, "MISSING"_el, "EMPTY"_el});
        REQUIRE_FALSE(changes.isUnchangedInheritance());
        auto result = changes.apply(base);
        REQUIRE_EQUAL(result.count(), el::unit::ItemCount{1});
        REQUIRE(result.contains("EMPTY"_el));
        REQUIRE(result.get("EMPTY"_el)->isEmpty());
        REQUIRE_FALSE(result.contains("MISSING"_el));
        changes.setInherit(Inherit::None).setVariable("NEW"_el, "value"_el);
        result = changes.apply(base);
        REQUIRE_EQUAL(result.count(), el::unit::ItemCount{1});
        REQUIRE_EQUAL(result.get("NEW"_el).value(), "value"_el);
        changes.clearVariableChanges().setInherit(el::text::StringList{});
        REQUIRE(changes.apply(base).count().isZero());
        changes.setInherit(Inherit::All);
        REQUIRE(changes.isUnchangedInheritance());
        REQUIRE(std::ranges::equal(changes.apply(base), base));
        REQUIRE_EQUAL(base.get("FIRST"_el).value(), "one"_el);
    }

    void testVariableActionsAndClearing() {
        const auto base = Map{{cAVariable, "base"_el}, {"B"_el, "other"_el}};
        auto changes = Changes{};
        changes.setVariable(cAVariable, "first"_el).setVariable(cAVariable, "last"_el);
        REQUIRE_EQUAL(changes.apply(base).get(cAVariable).value(), "last"_el);
        changes.removeVariable(cAVariable);
        REQUIRE_FALSE(changes.apply(base).contains(cAVariable));
        changes.setVariable(cAVariable, ""_el);
        REQUIRE(changes.apply(base).get(cAVariable)->isEmpty());
        changes.clearVariableChange(cAVariable).clearVariableChange("ABSENT"_el);
        REQUIRE(std::ranges::equal(changes.apply(base), base));
        changes.removeVariable(el::text::StringList{cAVariable, "B"_el, "ABSENT"_el});
        REQUIRE(changes.apply(base).count().isZero());
        changes.clearVariableChange(cAVariable);
        REQUIRE_EQUAL(changes.apply(base).count(), el::unit::ItemCount{1});
        changes.clearVariableChanges();
        REQUIRE(changes.isUnchangedInheritance());
    }

    void testIndependentCopiesAndOptions() {
        auto original = Changes{}.setInherit(el::text::StringList{cAVariable}).prependPath(Path{"front"_el});
        auto copy = original;
        copy.setInherit(Inherit::All).setVariable(cAVariable, "copy"_el).clearPathChanges();
        const auto base = Map{{cAVariable, "original"_el}, {"B"_el, "other"_el}};
        REQUIRE_FALSE(original.apply(base).contains("B"_el));
        REQUIRE_EQUAL(original.apply(base).get(cPathVariable).value(), "front"_el);
        REQUIRE_EQUAL(copy.apply(base).get(cAVariable).value(), "copy"_el);
        auto options = el::system::SubprocessOptions{}.setEnvironmentChanges(original);
        original.clearPathChanges();
        REQUIRE_EQUAL(options.environmentChanges().apply(base).get(cPathVariable).value(), "front"_el);
        options.environmentChanges().setVariable(cAVariable, "options"_el);
        const auto &constant = options;
        REQUIRE_EQUAL(constant.environmentChanges().apply(base).get(cAVariable).value(), "options"_el);
    }

    void testPathOrderRemovalsAndDeduplication() {
        auto changes = Changes{};
        changes.prependPath(Path{"first"_el})
            .prependPath(Path{"second"_el})
            .appendPath(Path{"tail"_el})
            .appendPath(Path{"last"_el})
            .removePath(Path{"gone"_el})
            .removePathsIn(Path{"sdk"_el});
        WITH_CONTEXT(requirePath(
            changes, "gone:sdk:sdk/bin:sdk-old:first:keep:keep/.:tail"_el, "second:first:sdk-old:keep:tail:last"_el));
        changes.prependPath(Path{"sdk/bin"_el});
        WITH_CONTEXT(requirePath(changes, "sdk/bin:sdk/other"_el, "sdk/bin:second:first:tail:last"_el));
        changes.clearPathChanges();
        WITH_CONTEXT(requirePath(changes, "a:a"_el, "a:a"_el));
        changes.setVariable(cPathVariable, pathText("x:x"_el));
        WITH_CONTEXT(requirePath(changes, "old"_el, "x:x"_el));
        changes.removePath(Path{"x"_el});
        WITH_CONTEXT(requirePath(changes, "old"_el, ""_el));
        changes.clearVariableChanges();
        WITH_CONTEXT(requirePath(changes, "x:keep"_el, "keep"_el));
    }

    void testEmptyAbsentAndRelativePaths() {
        auto changes = Changes{}.removePath(Path{"unrelated"_el});
        WITH_CONTEXT(requirePath(changes, "a::.:a/.:b:"_el, "a::b"_el));
        WITH_CONTEXT(requirePath(changes, ""_el, ""_el));
        REQUIRE_FALSE(changes.apply({}).contains(cPathVariable));
        changes.removeVariable(cPathVariable);
        REQUIRE_FALSE(changes.apply(Map{{cPathVariable, "old"_el}}).contains(cPathVariable));
        changes.appendPath(Path{"new"_el});
        WITH_CONTEXT(requirePath(changes, "old"_el, "new"_el));
        changes.clearVariableChanges().clearPathChanges().removePathsIn(Path{"."_el});
        WITH_CONTEXT(requirePath(changes, "a:../escape:.:"_el, "../escape"_el));
        changes.clearPathChanges().removePath(Path{"a/c"_el});
        WITH_CONTEXT(requirePath(changes, "a/b/../c:a/c-other:../a/c"_el, "a/c-other:../a/c"_el));
    }

    void testPlatformPolicies() {
        auto changes = Changes{}
                           .setInherit(el::text::StringList{"Path"_el, "home"_el})
                           .setVariable("HOME"_el, "new"_el)
                           .removePath(Path{"gone"_el});
        const auto &backend = Backend::native();
#ifdef EL_OS_WINDOWS
        const auto result = changes.apply(Map{{cPathVariable, "A;a;gone;B;B/."_el}, {"Home"_el, "old"_el}});
        REQUIRE_EQUAL(result.count(), el::ItemCount{2});
        REQUIRE_EQUAL(result.get(cPathVariable).value(), "A;B"_el);
        REQUIRE_EQUAL(result.get("Home"_el).value(), "new"_el);
        REQUIRE(backend.samePath(backend.pathKey("C:/SDK/bin/.."_el), backend.pathKey(R"/(c:\sdk)/"_el)));
        REQUIRE(backend.inBase(backend.pathKey("C:/SDK/bin"_el), backend.pathKey("c:/sdk"_el)));
        REQUIRE_FALSE(backend.inBase(backend.pathKey("c:/sdk-old"_el), backend.pathKey("c:/sdk"_el)));
        REQUIRE_FALSE(backend.inBase(backend.pathKey("d:/sdk"_el), backend.pathKey("c:/sdk"_el)));
        REQUIRE_FALSE(backend.inBase(backend.pathKey("sdk/bin"_el), backend.pathKey("c:/sdk"_el)));
#else
        const auto result = changes.apply(Map{{cPathVariable, "A:a"_el}, {"Home"_el, "old"_el}});
        REQUIRE_EQUAL(result.count(), el::ItemCount{1});
        REQUIRE_EQUAL(result.get("HOME"_el).value(), "new"_el);
        WITH_CONTEXT(requirePath(Changes{}.removePath(Path{"gone"_el}), "A:a:gone"_el, "A:a"_el));
        REQUIRE_FALSE(backend.samePath(backend.pathKey("/Tools"_el), backend.pathKey("/tools"_el)));
        REQUIRE(backend.samePath(backend.pathKey("/a/../../b"_el), backend.pathKey("/b"_el)));
        REQUIRE(backend.inBase(backend.pathKey("/a/b"_el), backend.pathKey("/"_el)));
        REQUIRE_FALSE(backend.inBase(backend.pathKey("/a/b"_el), backend.pathKey("a"_el)));
#endif
    }

    void testNativePlatformSelection() {
#ifdef EL_OS_WINDOWS
        REQUIRE_EQUAL(Backend::native().pathSeparator(), el::Char{U';'});
#else
        REQUIRE_EQUAL(Backend::native().pathSeparator(), el::Char{U':'});
#endif
    }

    void testNativeNameActions() {
        auto changes = Changes{}.setVariable("NAME"_el, "first"_el).setVariable("name"_el, "second"_el);
        const auto result = changes.apply({});
        if (Backend::native().pathSeparator() == el::Char{U';'}) {
            REQUIRE_EQUAL(result.count(), el::unit::ItemCount{1});
            changes.clearVariableChange("NaMe"_el);
            REQUIRE(changes.isUnchangedInheritance());
        } else {
            REQUIRE_EQUAL(result.count(), el::unit::ItemCount{2});
            changes.clearVariableChange("NAME"_el);
            REQUIRE_EQUAL(changes.apply({}).get("name"_el).value(), "second"_el);
        }
    }

    void testMalformedPathEntriesAreDroppedAndUntouchedValuesSurvive() {
        const auto bytes = el::unittest::th::stdStringFromHex("FF 61 FE");
        const auto opaque = el::text::String{std::string_view{bytes}};
        const auto basePath = el::join(pathText("a:"_el), opaque, pathText(":b"_el));
        const auto result =
            Changes{}.removePath(Path{"a"_el}).apply(Map{{cPathVariable, basePath}, {"RAW"_el, opaque}});
        const auto expected = el::text::String{"b"_el};
        REQUIRE(
            std::ranges::equal(
                el::text::impl::UnsafeU8StringAccess{result.get(cPathVariable).value()}.dataSpan(),
                el::text::impl::UnsafeU8StringAccess{expected}.dataSpan()));
        REQUIRE(
            std::ranges::equal(
                el::text::impl::UnsafeU8StringAccess{result.get("RAW"_el).value()}.dataSpan(),
                el::text::impl::UnsafeU8StringAccess{opaque}.dataSpan()));
        const auto otherBytes = el::unittest::th::stdStringFromHex("FE 61 FF");
        const auto otherOpaque = el::text::String{std::string_view{otherBytes}};
        const auto distinctPath = el::join(opaque, pathText(":"_el), otherOpaque, pathText(":"_el), opaque);
        const auto distinctResult =
            Changes{}.removePath(Path{"unrelated"_el}).apply(Map{{cPathVariable, distinctPath}});
        const auto distinctExpected = el::text::String{};
        REQUIRE(
            std::ranges::equal(
                el::text::impl::UnsafeU8StringAccess{distinctResult.get(cPathVariable).value()}.dataSpan(),
                el::text::impl::UnsafeU8StringAccess{distinctExpected}.dataSpan()));
        const auto raw16 = std::u16string{char16_t{0xD800}, u'x'};
        const auto nativeOpaque = el::text::U16String{std::u16string_view{raw16}};
        auto native = Processor::Environment<el::text::U16String>{};
        native.emplace(u"=C:"_el, u"C:\\working"_el);
        native.emplace(u"RAW"_el, nativeOpaque);
        native.emplace(
            u"PATH"_el,
            el::join(
                el::StringConverter{pathText("gone:"_el)}.toU16String(),
                nativeOpaque,
                el::StringConverter{pathText(":keep"_el)}.toU16String()));
        REQUIRE(std::ranges::equal(Processor::apply(Changes{}, native), native));
        const auto processed = Processor::apply(Changes{}.removePath(Path{"gone"_el}), native);
        REQUIRE(processed.contains(u"=C:"_el));
        REQUIRE(
            std::ranges::equal(
                el::text::impl::UnsafeU16StringAccess{processed.at(u"RAW"_el)}.dataSpan(),
                el::text::impl::UnsafeU16StringAccess{nativeOpaque}.dataSpan()));
        const auto expected16 = el::text::U16String{u"keep"_el};
        REQUIRE(
            std::ranges::equal(
                el::text::impl::UnsafeU16StringAccess{processed.at(u"PATH"_el)}.dataSpan(),
                el::text::impl::UnsafeU16StringAccess{expected16}.dataSpan()));
    }

    void testInvalidConfigurationFailsImmediately() {
        auto changes = Changes{}.setVariable(cAVariable, "original"_el);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.setVariable(""_el, "value"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.setVariable("A=B"_el, "value"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.setVariable("A\0B"_el, "value"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.setVariable(cAVariable, "bad\0value"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.setInherit(el::text::StringList{cAVariable, ""_el}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.removeVariable(el::text::StringList{cAVariable, ""_el}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.removeVariable("A\0B"_el));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, changes.removeVariable(el::text::StringList{cAVariable, "A\0B"_el}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.clearVariableChange("bad=name"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.clearVariableChange("A\0B"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.setInherit(static_cast<Inherit>(255)));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.prependPath(Path{}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.appendPath(Path{}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.removePath(Path{}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.removePathsIn(Path{}));
        auto separated = el::join("a"_el, el::text::String::fromCharacter(Backend::native().pathSeparator()), "b"_el);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, changes.appendPath(Path{separated}));
        const auto bad = el::unittest::th::stdStringFromHex("FF FE");
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, changes.setVariable("BAD"_el, el::text::String{std::string_view{bad}}));
        REQUIRE_EQUAL(changes.apply({}).get(cAVariable).value(), "original"_el);
    }
};
