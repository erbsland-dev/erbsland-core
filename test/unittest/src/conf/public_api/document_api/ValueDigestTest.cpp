// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/impl/value/Document.hpp>
#include <erbsland/conf/impl/value/Value.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <bit>
#include <cstdint>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(Value ValueDigest)
class ValueDigestTest final : public el::UnitTest {
private:
    using ValueImpl = el::conf::impl::Value;
    using StoredValue = el::conf::impl::ValuePtr;
    static constexpr auto cAlgorithm = el::cryptology::HashAlgorithm{el::cryptology::HashAlgorithm::Sha2_256};
    static constexpr auto cSampleValuePath = "sample.value"_el;
    static constexpr auto cSampleName = "sample"_el;

    auto parse(const el::String &text) -> el::conf::DocumentPtr { return el::conf::Parser{}.parseTextOrThrow(text); }

    void requireDifferent(const el::conf::ConstValuePtr &first, const el::conf::ConstValuePtr &second) {
        REQUIRE_NOT_EQUAL(first->digest(cAlgorithm), second->digest(cAlgorithm));
    }

public:
    void testRepeatabilityAndEquivalentSourceSpelling() {
        const auto first = parse("[sample]\nFirst Value: 42\nsecond: \"text\"\n"_el);
        const auto second = parse(
            "# Moved source locations and changed whitespace.\n[sample]\nfirst_value = 42\n\nsecond = \"text\" # Comment\n"_el);
        REQUIRE_EQUAL(first->digest(cAlgorithm), first->digest(cAlgorithm));
        REQUIRE_EQUAL(first->digest(cAlgorithm), second->digest(cAlgorithm));
        REQUIRE_EQUAL(
            first->valueOrThrow(cSampleName)->digest(cAlgorithm),
            second->valueOrThrow(cSampleName)->digest(cAlgorithm));
    }

    void testNamesContentsOrderTypesAndStructure() {
        const auto original = parse("[sample]\nfirst: 1\nsecond: 2\n"_el);
        for (
            const auto &text :
            {"[renamed]\nfirst: 1\nsecond: 2\n"_el,
                "[sample]\nrenamed: 1\nsecond: 2\n"_el,
                "[sample]\nfirst: 3\nsecond: 2\n"_el,
                "[sample]\nfirst: 1\n"_el,
                "[sample]\nfirst: 1\nsecond: 2\nthird: 3\n"_el,
                "[sample]\nsecond: 2\nfirst: 1\n"_el,
                "[sample]\nfirst: \"1\"\nsecond: 2\n"_el,
                "[sample]\nfirst: 1.0\nsecond: 2\n"_el,
                "[sample.first]\nsecond: 2\n"_el}) {
            WITH_CONTEXT(requireDifferent(original, parse(text)));
        }
        WITH_CONTEXT(requireDifferent(parse("[sample]\nvalues: 1, 2\n"_el), parse("[sample]\nvalues: 2, 1\n"_el)));
        WITH_CONTEXT(requireDifferent(
            parse("*[sample]*\nvalue: 1\n*[sample]*\nvalue: 2\n"_el),
            parse("*[sample]*\nvalue: 2\n*[sample]*\nvalue: 1\n"_el)));
    }

    void testOwnNameAndAncestorIndependence() {
        const auto firstDocument = parse("[first.sample]\nvalue: 1\n"_el);
        const auto secondDocument = parse("[second.sample]\nvalue: 1\n"_el);
        const auto first = firstDocument->valueOrThrow("first.sample"_el);
        const auto second = secondDocument->valueOrThrow("second.sample"_el);
        const auto renamed = parse("[second.changed]\nvalue: 1\n"_el)->valueOrThrow("second.changed"_el);
        REQUIRE_EQUAL(first->digest(cAlgorithm), second->digest(cAlgorithm));
        WITH_CONTEXT(requireDifferent(first, renamed));
        const auto regular = ValueImpl::createText("same"_el);
        const auto text = ValueImpl::createText("same"_el);
        regular->setName(el::conf::Name::createRegular(cSampleName));
        text->setName(el::conf::Name::createText(cSampleName));
        WITH_CONTEXT(requireDifferent(regular, text));
        text->setName(el::conf::Name::createIndex(0));
        WITH_CONTEXT(requireDifferent(regular, text));
        const auto index = text->digest(cAlgorithm);
        text->setName(el::conf::Name::createIndex(1));
        REQUIRE_NOT_EQUAL(index, text->digest(cAlgorithm));
        text->setName(el::conf::Name::createTextIndex(0));
        REQUIRE_NOT_EQUAL(index, text->digest(cAlgorithm));
    }

    void testLengthAndContainerFraming() {
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createValueList({ValueImpl::createText("ab"_el), ValueImpl::createText("c"_el)}),
            ValueImpl::createValueList({ValueImpl::createText("a"_el), ValueImpl::createText("bc"_el)})));
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createValueList(
                {ValueImpl::createValueList({ValueImpl::createInteger(1)}), ValueImpl::createInteger(2)}),
            ValueImpl::createValueList(
                {ValueImpl::createValueList({ValueImpl::createInteger(1), ValueImpl::createInteger(2)})})));
        WITH_CONTEXT(requireDifferent(ValueImpl::createText("a\0b"_el), ValueImpl::createText("a"_el)));
        const auto first = ValueImpl::createText("same"_el);
        const auto second = ValueImpl::createText("same"_el);
        first->setName(el::conf::Name::createText("ab"_el));
        second->setName(el::conf::Name::createText("a"_el));
        WITH_CONTEXT(requireDifferent(first, second));
        const auto empty = std::vector<el::conf::ConstValuePtr>{
            ValueImpl::createText({}),
            ValueImpl::createBytes({}),
            ValueImpl::createValueList({}),
            ValueImpl::createSectionList(),
            ValueImpl::createIntermediateSection(),
            ValueImpl::createSectionWithNames(),
            ValueImpl::createSectionWithTexts(),
            std::make_shared<el::conf::impl::Document>()};
        for (auto i = std::size_t{}; i < empty.size(); ++i) {
            for (auto j = i + 1; j < empty.size(); ++j) {
                WITH_CONTEXT(requireDifferent(empty[i], empty[j]));
            }
        }
    }

    void testEveryScalarStorageAndAllAlgorithms() {
        using namespace el::time;
        const auto date = Date::fromYearMonthDay(2026, 10, 30);
        const auto time = Time{Hour{12}, Minute{34}, Second{56}, Nanoseconds{123456789}};
        const auto values = std::vector<StoredValue>{
            ValueImpl::createInteger(-42),
            ValueImpl::createBoolean(true),
            ValueImpl::createFloat(2.5),
            ValueImpl::createText("text\0payload"_el),
            ValueImpl::createBytes(el::mem::ByteBlock{0, 1, 255}),
            ValueImpl::createDate(date),
            ValueImpl::createTime(time),
            ValueImpl::createTimeWithZone(TimeWithZone{time, TimeZone{Hours{2}}}),
            ValueImpl::createDateTime(DateTime{date, time, Seconds{3600}}),
            ValueImpl::createCalendarDelta(CalendarDelta{Months{3}}),
            ValueImpl::createRegEx(el::re::RegEx::lazyCompile("[a-z]+"_el))};
        for (const auto &value : values) {
            for (const auto algorithm : el::cryptology::HashAlgorithm::all()) {
                WITH_CONTEXT(runWithContext(SOURCE_LOCATION(), [&]() -> void {
                    const auto digest = value->digest(algorithm);
                    REQUIRE_EQUAL(digest.length(), algorithm.digestSize());
                    REQUIRE_EQUAL(digest, value->deepCopy()->digest(algorithm));
                    REQUIRE_EQUAL(digest, value->digest(algorithm));
                }));
            }
        }
        WITH_CONTEXT(requireDifferent(ValueImpl::createBoolean(false), ValueImpl::createBoolean(true)));
        WITH_CONTEXT(requireDifferent(ValueImpl::createFloat(2.5), ValueImpl::createFloat(2.75)));
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createBytes(el::mem::ByteBlock{0, 1}), ValueImpl::createBytes(el::mem::ByteBlock{0, 2})));
        const auto invalid = el::cryptology::HashAlgorithm{static_cast<el::cryptology::HashAlgorithm::Value>(255)};
        REQUIRE_THROWS_AS(el::err::LogicFailure, values.front()->digest(invalid));
        REQUIRE_THROWS_AS(el::err::LogicFailure, std::make_shared<el::conf::impl::Document>()->digest(invalid));
    }

    void testFloatingPointBitsAndTemporalComponents() {
        using namespace el::time;
        WITH_CONTEXT(requireDifferent(ValueImpl::createFloat(0.0), ValueImpl::createFloat(-0.0)));
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createFloat(std::bit_cast<double>(uint64_t{0x7ff8000000000001})),
            ValueImpl::createFloat(std::bit_cast<double>(uint64_t{0x7ff8000000000002}))));
        const auto date = Date::fromYearMonthDay(2026, 10, 30);
        const auto midnight = Time{};
        WITH_CONTEXT(
            requireDifferent(ValueImpl::createDate(date), ValueImpl::createDate(Date::fromYearMonthDay(2026, 10, 31))));
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createTime(midnight), ValueImpl::createTime(Time{Hour{}, Minute{}, Second{}, Nanoseconds{1}})));
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createTime(midnight), ValueImpl::createTimeWithZone(TimeWithZone{midnight, TimeZone::utc()})));
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createTimeWithZone(TimeWithZone{midnight, TimeZone::utc()}),
            ValueImpl::createTimeWithZone(TimeWithZone{midnight, TimeZone{Hours{1}}})));
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createTimeWithZone(TimeWithZone{midnight, TimeZone::fromNameOrThrow("Europe/Zurich"_el)}),
            ValueImpl::createTimeWithZone(TimeWithZone{midnight, TimeZone::fromNameOrThrow("Europe/London"_el)})));
        const auto instant = DateTime{date, midnight};
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createDateTime(instant), ValueImpl::createDateTime(instant.toTimeZone(TimeZone{Hours{1}}))));
        const auto zero = ValueImpl::createCalendarDelta(CalendarDelta{});
        const auto deltas = std::vector<CalendarDelta>{
            CalendarDelta{Nanoseconds{1}},
            CalendarDelta{Microseconds{1}},
            CalendarDelta{Milliseconds{1}},
            CalendarDelta{Seconds{1}},
            CalendarDelta{Minutes{1}},
            CalendarDelta{Hours{1}},
            CalendarDelta{Days{1}},
            CalendarDelta{Weeks{1}},
            CalendarDelta{Months{1}},
            CalendarDelta{Years{1}}};
        for (const auto &delta : deltas) {
            WITH_CONTEXT(requireDifferent(zero, ValueImpl::createCalendarDelta(delta)));
        }
        WITH_CONTEXT(requireDifferent(
            ValueImpl::createCalendarDelta(CalendarDelta{Days{7}}),
            ValueImpl::createCalendarDelta(CalendarDelta{Weeks{1}})));
    }

    void testRegexFlagsPatternsAndCompilationState() {
        const auto regex = el::re::RegEx::lazyCompile("a b"_el, el::re::Flag::Verbose);
        const auto value = ValueImpl::createRegEx(regex);
        const auto digest = value->digest(cAlgorithm);
        REQUIRE_FALSE(regex->isCompiled());
        WITH_CONTEXT(requireDifferent(value, ValueImpl::createRegEx(el::re::RegEx::lazyCompile("a b"_el))));
        WITH_CONTEXT(requireDifferent(
            value, ValueImpl::createRegEx(el::re::RegEx::lazyCompile("a c"_el, el::re::Flag::Verbose))));
        regex->compileNow();
        REQUIRE_EQUAL(value->digest(cAlgorithm), digest);
        REQUIRE_EQUAL(
            ValueImpl::createRegEx(el::re::RegEx::compile("a b"_el, el::re::Flag::Verbose))->digest(cAlgorithm),
            digest);
        const auto invalid = el::re::RegEx::lazyCompile("("_el);
        REQUIRE_NOTHROW(ValueImpl::createRegEx(invalid)->digest(cAlgorithm));
        REQUIRE_FALSE(invalid->isCompiled());
        for (const auto &pattern : {el::text::AnyString{u"[a-z]+"_el}, el::text::AnyString{U"[a-z]+"_el}}) {
            const auto expression = el::re::RegEx::lazyCompile(pattern);
            const auto wide = ValueImpl::createRegEx(expression);
            const auto expected = wide->digest(cAlgorithm);
            REQUIRE_FALSE(expression->isCompiled());
            expression->compileNow();
            REQUIRE_EQUAL(wide->digest(cAlgorithm), expected);
        }
    }

    void testValidationMetadataAndInsertedDefaults() {
        using namespace el::conf::vr;
        using namespace el::conf::vr::builder;
        const auto document = parse("[sample]\nvalue: \"secret\"\n"_el);
        const auto digest = document->digest(cAlgorithm);
        auto builder = RulesBuilder{};
        builder.addRule(cSampleName, RuleType::Section, Title{"Metadata"_el});
        builder.addRule(cSampleValuePath, RuleType::Text, IsSecret{});
        builder.takeRules()->validate(document, 1);
        REQUIRE_EQUAL(document->digest(cAlgorithm), digest);
        const auto value = document->valueOrThrow(cSampleValuePath);
        value->setLocation({});
        REQUIRE_EQUAL(document->digest(cAlgorithm), digest);
        auto binary = el::mem::ByteBlock{0, 1, 2};
        const auto bytes = ValueImpl::createBytes(binary);
        const auto byteDigest = bytes->digest(cAlgorithm);
        binary.markAsSensitive();
        REQUIRE_EQUAL(bytes->digest(cAlgorithm), byteDigest);
        const auto stored = std::dynamic_pointer_cast<ValueImpl>(value);
        stored->markAsDefaultValue();
        REQUIRE_EQUAL(document->digest(cAlgorithm), digest);
        builder.addRule(cSampleName, RuleType::Section);
        builder.addRule(cSampleValuePath, RuleType::Text);
        builder.addRule("sample.inserted"_el, RuleType::Integer, Default{42});
        const auto withDefaults = parse("[sample]\nvalue: \"secret\"\n"_el);
        builder.takeRules()->validate(withDefaults, 1);
        REQUIRE_NOT_EQUAL(withDefaults->digest(cAlgorithm), digest);
    }

    void testDeepTreeUsesIterativeTraversal() {
        auto root = ValueImpl::createSectionWithNames();
        auto nodes = std::vector<StoredValue>{root};
        for (auto depth = 0; depth < 512; ++depth) {
            auto child = ValueImpl::createSectionWithNames();
            child->setName(el::conf::Name::createRegular("child"_el));
            nodes.back()->addValue(child);
            nodes.push_back(child);
        }
        const auto digest = root->digest(cAlgorithm);
        REQUIRE_EQUAL(root->digest(cAlgorithm), digest);
        nodes.back()->addValue(ValueImpl::createInteger(1));
        REQUIRE_NOT_EQUAL(root->digest(cAlgorithm), digest);
    }
};
