// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CharSet.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/anti_patterns/RuleRegistry.hpp>
#include <tool/anti_patterns/SourceIndex.hpp>

using namespace el::text::literals;

TESTED_TARGETS(UnnecessaryUnitConversionRule UnnecessaryTypeConstructionRule NamespaceNoiseRule)
class AntiPatternsExpressionTest final : public el::UnitTest {
public: // implement UnitTest
    auto additionalErrorMessages() -> std::string override { return el::StringConverter{_sourceText}.toStdString(); }

public: // tests
    void testUnitComparisons() {
        for (
            const auto &expression : el::StringList{
                "x.count().toSizeT() >= 16U;"_el,
                "x.length().toSizeT() < 0x10UL;"_el,
                "x.toSizeT() == 0;"_el,
                "x->toSizeT() != 0b10u;"_el,
                "if (x /* unit */ . toSizeT ( ) <= 16) {}"_el,
                "return x.toSizeT() > 16 && ready;"_el}) {
            WITH_CONTEXT(check("unnecessary_unit_conversion"_el, expression, el::ItemCount{1}));
        }
        for (
            const auto &expression : el::StringList{
                "native(x.toSizeT());"_el,
                "x.toSizeT() >= nativeSize;"_el,
                "x.toSizeTOrThrow() >= 16U;"_el,
                "x.toSizeT() >= 16U + extra;"_el,
                "x.toSizeT() >= 1.5;"_el,
                "x.toSizeT() >= -1;"_el,
                "x.toSizeT() >= 16_custom;"_el,
                "// x.toSizeT() >= 16;\n"_el,
                "auto s = R\"(x.toSizeT() >= 16;)\";"_el,
                "#define CHECK x.toSizeT() >= 16\n"_el,
                "x.toSizeT(\n#ifdef VALUE\n) >= 16;\n#endif\n"_el}) {
            WITH_CONTEXT(check("unnecessary_unit_conversion"_el, expression, el::ItemCount{0}));
        }
    }

    void testKnownConstructionForms() {
        for (
            const auto &expression : el::StringList{
                "CharSet{Char{U'x'}};"_el,
                "el::CharSet{el::Char{U'x'}};"_el,
                "text::CharSet{text::Char{U'x'}};"_el,
                "erbsland::text::CharSet{erbsland::text::Char{U'x'}};"_el,
                "text.contains(el::String{\"x\"_el});"_el,
                "text.startsWith(String{R\"(x)\"_el});"_el,
                "text->endsWith(text::String{\"x\"_el}, compare);"_el,
                "text.find(String{\"x\"_el}, start);"_el,
                "text.slice(el::ByteRange{el::ByteIndex{0}, el::ByteCount{2}});"_el,
                "text.slice(unit::ByteRange{unit::ByteIndex{begin()}, unit::ByteIndex{end()}});"_el}) {
            WITH_CONTEXT(check("unnecessary_type_construction"_el, expression, el::ItemCount{1}));
        }
        for (
            const auto &expression : el::StringList{
                "other::CharSet{Char{U'x'}};"_el,
                "CharSet{other::Char{U'x'}};"_el,
                "CharSet{Char{value}};"_el,
                "CharSet{Char{U'x'}, range};"_el,
                "foo(el::String{\"x\"_el});"_el,
                "text.contains(other::String{\"x\"_el});"_el,
                "text.contains(String{\"x\"});"_el,
                "text.contains(String{value});"_el,
                "text.contains(String{\"x\"_el}.trimmed());"_el,
                "text.slice(ByteRange{});"_el,
                "text.slice(ByteRange{begin, end});"_el,
                "text.slice(ByteRange{ByteIndex{0}, ByteCount{2}}.clampedTo(bounds));"_el,
                "// CharSet{Char{U'x'}};\n"_el,
                "auto s = R\"(CharSet{Char{U'x'}})\";"_el,
                "#define SET CharSet{Char{U'x'}}\n"_el,
                "text.contains(String{"_el}) {
            WITH_CONTEXT(check("unnecessary_type_construction"_el, expression, el::ItemCount{0}));
        }
    }

    void testNamespaceThresholdAndScopes() {
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace erbsland::conf { text::A a; text::B b; text::C c; }"_el,
            el::ItemCount{0}));
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace erbsland::conf { text::A a; text::B b; text::C c; text::D d; }"_el,
            el::ItemCount{1}));
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace sample { text::A a; unit::B b; err::C c; mem::D d; }"_el,
            el::ItemCount{0}));
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace a { text::A a; text::B b; } namespace b { text::C c; text::D d; }"_el,
            el::ItemCount{0}));
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace a { text::A a; text::B b; namespace b { text::C c; text::D d; } }"_el,
            el::ItemCount{0}));
        WITH_CONTEXT(check("namespace_noise"_el, "text::A a; text::B b; text::C c; text::D d;"_el, el::ItemCount{0}));
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace a { using namespace text::literals; using text::A; using B = text::B; namespace alias = text::detail; }"_el,
            el::ItemCount{0}));
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace a { other::text::A a; other::text::B b; other::text::C c; other::text::D d; }"_el,
            el::ItemCount{0}));
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace a { /* text::A text::B text::C text::D */ auto s = \"text::A text::B text::C text::D\"; }"_el,
            el::ItemCount{0}));
        WITH_CONTEXT(check(
            "namespace_noise"_el,
            "namespace a {\n#ifdef ERBSLAND_OS_WINDOWS\ntext::A a; text::B b; text::C c; text::D d;\n#endif\n}"_el,
            el::ItemCount{0}));
        for (
            const auto &domain :
            el::StringList{"text"_el, "unit"_el, "err"_el, "mem"_el, "options"_el, "path"_el, "stream"_el}) {
            const auto text = el::join(
                "namespace a { void f() { "_el,
                domain,
                "::A a; "_el,
                domain,
                "::B b; el::"_el,
                domain,
                "::C c; erbsland::"_el,
                domain,
                "::D d; } }"_el);
            WITH_CONTEXT(check("namespace_noise"_el, text, el::ItemCount{1}));
            WITH_CONTEXT(check("namespace_noise"_el, text, el::ItemCount{0}, "src/Example.hpp"_el));
            WITH_CONTEXT(check("namespace_noise"_el, text, el::ItemCount{0}, "src/Example.tpp"_el));
        }
    }

    void testSuggestedFormsCompileAndPreserveResults() {
        const auto text = el::String{"xyz"_el};
        REQUIRE((el::CharSet{U'x'}.contains(U'x')));
        REQUIRE(text.contains("x"_el));
        REQUIRE(text.startsWith("x"_el));
        REQUIRE(text.endsWith("z"_el));
        REQUIRE(text.find("y"_el) == el::ByteIndex{1});
        REQUIRE(text.slice({el::ByteIndex{0}, el::ByteCount{2}}) == "xy"_el);
        REQUIRE(text.length() >= el::ByteCount{2});
    }

private:
    void check(
        const el::String &rule,
        const el::String &text,
        el::ItemCount expected,
        const el::String &path = "src/Example.cpp"_el) {
        _sourceText = text;
        const auto source = tool::anti_patterns::SourceIndex{el::Path{path}, text};
        auto registry = tool::anti_patterns::RuleRegistry{};
        const auto configuration = el::conf::Parser{}.parseTextOrThrow(
            "[anti_patterns.namespace_noise]\n"
            "domains: \"text\", \"unit\", \"err\", \"mem\", \"options\", \"path\", \"stream\"\n"_el);
        registry.configure(configuration->valueOrThrow("anti_patterns"_el));
        auto findings = el::List<el::ByteRange>{};
        const auto *detector = registry.find(rule);
        REQUIRE(detector != nullptr);
        detector->scan(source, findings);
        REQUIRE_EQUAL(findings.count(), expected);
        for (const auto &range : findings) {
            REQUIRE(range.isWithin(text.length()));
            REQUIRE_FALSE(source.slice(range).isEmpty());
        }
    }

private:
    el::String _sourceText; ///< Source of the current assertion.
};
