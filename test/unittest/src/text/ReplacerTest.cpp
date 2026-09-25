// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/placeholder/Filter.hpp>
#include <erbsland/text/placeholder/Replacer.hpp>
#include <erbsland/text/placeholder/ReplacerError.hpp>
#include <erbsland/text/placeholder/Source.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringMap.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
namespace ph = el::text::placeholder;

TESTED_TARGETS(Replacer ReplacerOptions ReplacerError ReplacerErrorCategory Source Filter EscapeMode)
class ReplacerTest final : public el::UnitTest {
    class EchoSource final : public ph::Source {
    public:
        [[nodiscard]] auto sourceNames() const -> el::text::StringList override {
            return el::text::StringList{"echo"_el, "second source"_el};
        }
        [[nodiscard]] auto resolve(const el::text::String &, const el::text::String &parameter)
            -> el::text::String override {
            if (parameter == "bad"_el) {
                throw ph::ReplacerError{ph::ReplacerErrorCategory::ValueNotFound, "Rejected echo value."_el};
            }
            return parameter == "recursive"_el ? "${echo:next}"_el : parameter;
        }
    };
    class BracketFilter final : public ph::Filter {
    public:
        [[nodiscard]] auto filterNames() const -> el::text::StringList override {
            return el::text::StringList{"bracket"_el};
        }
        [[nodiscard]] auto apply(
            const el::text::String &, const el::text::String &parameter, const el::text::String &value)
            -> el::text::String override {
            return el::text::String::fromJoined({"["_el, parameter, value, "]"_el});
        }
    };
    class SyntaxSource final : public ph::Source {
    public:
        [[nodiscard]] auto sourceNames() const -> el::text::StringList override {
            return el::text::StringList{"syntax"_el};
        }
        [[nodiscard]] auto resolve(const el::text::String &, const el::text::String &) -> el::text::String override {
            ++resolveCount;
            return "value"_el;
        }
        [[nodiscard]] auto validate(const el::text::String &, const el::text::String &parameter) -> bool override {
            ++validateCount;
            return parameter == "good"_el;
        }
        int resolveCount{};
        int validateCount{};
    };
    class SyntaxFilter final : public ph::Filter {
    public:
        [[nodiscard]] auto filterNames() const -> el::text::StringList override {
            return el::text::StringList{"check"_el};
        }
        [[nodiscard]] auto apply(const el::text::String &, const el::text::String &, const el::text::String &value)
            -> el::text::String override {
            ++applyCount;
            return value;
        }
        [[nodiscard]] auto validate(const el::text::String &, const el::text::String &parameter) -> bool override {
            ++validateCount;
            return parameter == "good"_el;
        }
        int applyCount{};
        int validateCount{};
    };

public:
    void testDefaultsAndRegistry() {
        auto replacer = ph::Replacer{};
        REQUIRE_THROWS_AS(el::err::LogicError, replacer.validate("plain"_el));
        REQUIRE_THROWS_AS(el::err::LogicError, replacer.validateOrThrow("plain"_el));
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(replacer.replace("plain"_el)));
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(replacer.replaceOrThrow("plain"_el)));
        const auto source = std::make_shared<EchoSource>();
        const auto filter = std::make_shared<BracketFilter>();
        replacer.addSource(source);
        replacer.addFilter(filter);
        REQUIRE_EQUAL(replacer.source("SECOND_SOURCE"_el), source);
        REQUIRE_EQUAL(replacer.filter("BRACKET"_el), filter);
        REQUIRE_EQUAL(replacer.replaceOrThrow("plain"_el), "plain"_el);
        REQUIRE_EQUAL(replacer.replaceOrThrow("a ${echo:hi|bracket:!} b"_el), "a [!hi] b"_el);
        REQUIRE_EQUAL(replacer.replaceOrThrow("${echo:recursive}"_el), "${echo:next}"_el);
        replacer.removeFilter(filter);
        replacer.removeSource(source);
        REQUIRE(replacer.filter("bracket"_el) == nullptr);
        REQUIRE(replacer.source("echo"_el) == nullptr);
    }

    void testBackslashAndDoubleEscaping() {
        auto replacer = ph::Replacer{};
        replacer.addSource(std::make_shared<EchoSource>());
        REQUIRE_EQUAL(replacer.replaceOrThrow(R"(\${echo:x} \\${echo:y} \q)"_el), R"(${echo:x} \y \q)"_el);
        REQUIRE_EQUAL(replacer.replaceOrThrow(R"(${echo:a\|b})"_el), "a|b"_el);
        REQUIRE_EQUAL(replacer.replaceOrThrow(R"(${echo:a\}b})"_el), "a}b"_el);
        auto options = ph::ReplacerOptions{};
        options.setEscapeMode(ph::EscapeMode::Double);
        auto doubled = ph::Replacer{options};
        doubled.addSource(std::make_shared<EchoSource>());
        REQUIRE_EQUAL(doubled.replaceOrThrow("$${echo:x} ${echo:a||b}"_el), "${echo:x} a|b"_el);
        REQUIRE_EQUAL(doubled.replaceOrThrow("${echo:a}}b}"_el), "a}b"_el);
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(ph::Replacer{options.setFrame("${"_el, ">>"_el)}));
    }

    void testSyntaxAndLimits() {
        auto options = ph::ReplacerOptions{};
        REQUIRE_THROWS_AS(el::err::ParameterError, options.setFrame({}, "}"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, options.setFilterSeparator("12345678901234567"_el));
        options.setFrame("[["_el, "]]"_el).setFilterSeparator("/"_el).setNameSeparator("="_el);
        auto replacer = ph::Replacer{options};
        replacer.addSource(std::make_shared<EchoSource>());
        REQUIRE_EQUAL(replacer.replaceOrThrow("[[echo=hello]]"_el), "hello"_el);
        options.setFilterSeparator({}).setNameSeparator({});
        auto bare = ph::Replacer{options};
        bare.addSource(std::make_shared<EchoSource>());
        REQUIRE_EQUAL(bare.replaceOrThrow("[[echo]]"_el), ""_el);
        REQUIRE_FALSE(bare.validate("[[echo=value]]"_el));
        auto overlapping = ph::Replacer{ph::ReplacerOptions{}.setFrame("{"_el, "||"_el).setFilterSeparator("|"_el)};
        overlapping.addSource(std::make_shared<EchoSource>());
        REQUIRE_EQUAL(overlapping.replaceOrThrow("{echo:x||"_el), "x"_el);
    }

    void testUnicodeDelimiters() {
        auto options = ph::ReplacerOptions{};
        options.setFrame("«"_el, "»"_el).setFilterSeparator("¦"_el).setNameSeparator("→"_el);
        auto replacer = ph::Replacer{options};
        replacer.addSource(std::make_shared<EchoSource>());
        replacer.addFilter(std::make_shared<BracketFilter>());
        REQUIRE_EQUAL(replacer.replaceOrThrow("before «echo→x¦bracket→!» after"_el), "before [!x] after"_el);
        options.setEscapeMode(ph::EscapeMode::Double).setFrame("««"_el, "»"_el);
        REQUIRE_THROWS_AS(el::err::LogicError, ph::Replacer{options});
    }

    void testValidationAndTolerance() {
        auto replacer = ph::Replacer{};
        replacer.addSource(std::make_shared<EchoSource>());
        replacer.addTextFilters();
        REQUIRE(replacer.validate("${echo:value|required}"_el));
        REQUIRE_FALSE(replacer.validate("${echo:bad}"_el));
        REQUIRE_FALSE(replacer.validate("${echo:x|lower:invalid}"_el));
        REQUIRE(replacer.validate("${echo:x|error_if:empty}"_el));
        REQUIRE_EQUAL(replacer.replace("${echo:good} ${echo:bad} ${echo:next}"_el), "good ${echo:bad} next"_el);
        REQUIRE_EQUAL(replacer.replace("${echo:unterminated"_el), "${echo:unterminated"_el);
        REQUIRE_EQUAL(replacer.replace("${echo:x\n}${echo:y}"_el), "${echo:x\n}y"_el);
        try {
            static_cast<void>(replacer.replaceOrThrow("abc ${unknown:x}"_el));
            REQUIRE(false);
        } catch (const ph::ReplacerError &error) {
            REQUIRE_EQUAL(error.category(), ph::ReplacerErrorCategory::Unsupported);
            REQUIRE(error.offset().has_value());
            REQUIRE_EQUAL(error.offset()->toSizeT(), 4U);
        }
        REQUIRE_THROWS_AS(ph::ReplacerError, replacer.validateOrThrow("${echo:x|lower:invalid}"_el));
        auto tooMany = el::text::StringEditor{"${echo:x"_el};
        for (auto index = 0U; index < 17U; ++index) {
            tooMany.append("|trim"_el);
        }
        tooMany.append("}"_el);
        REQUIRE_THROWS_AS(ph::ReplacerError, static_cast<void>(replacer.replaceOrThrow(el::text::String{tooMany})));
        auto maximum = el::text::StringEditor{"${echo:x"_el};
        for (auto index = 0U; index < 16U; ++index) {
            maximum.append("|trim"_el);
        }
        maximum.append("}"_el);
        REQUIRE(replacer.validate(el::text::String{maximum}));
    }

    void testValidationHooksAndOffsets() {
        auto replacer = ph::Replacer{};
        const auto source = std::make_shared<SyntaxSource>();
        const auto filter = std::make_shared<SyntaxFilter>();
        replacer.addSource(source);
        replacer.addFilter(filter);
        REQUIRE(replacer.validate("${syntax:good|check:good}"_el));
        REQUIRE_EQUAL(source->resolveCount, 0);
        REQUIRE_EQUAL(filter->applyCount, 0);
        REQUIRE_EQUAL(source->validateCount, 1);
        REQUIRE_EQUAL(filter->validateCount, 1);
        REQUIRE_FALSE(replacer.validate("${syntax:bad}"_el));
        REQUIRE_FALSE(replacer.validate("${syntax:good|check:bad}"_el));
        try {
            replacer.validateOrThrow("é ${syntax:good|check:bad}"_el);
            REQUIRE(false);
        } catch (const ph::ReplacerError &error) {
            REQUIRE_EQUAL(error.category(), ph::ReplacerErrorCategory::Syntax);
            REQUIRE(error.offset().has_value());
            REQUIRE_EQUAL(error.offset()->toSizeT(), 2U);
        }
        REQUIRE_EQUAL(replacer.replaceOrThrow("${syntax:bad|check:bad}"_el), "value"_el);
        REQUIRE_EQUAL(source->resolveCount, 1);
        REQUIRE_EQUAL(filter->applyCount, 1);
    }

    void testBuiltInVariableValidation() {
        auto replacer = ph::Replacer{};
        replacer.setVariableSource(el::text::StringMap<el::text::String>{{{"Name"_el, "first"_el}}}, "settings"_el);
        REQUIRE(replacer.validate("${settings:missing}"_el));
        REQUIRE_EQUAL(replacer.replaceOrThrow("${SETTINGS:name}"_el), "first"_el);
        replacer.setVariableSource(el::text::StringMap<el::text::String>{{{"name"_el, "second"_el}}}, "SETTINGS"_el);
        REQUIRE_EQUAL(replacer.replaceOrThrow("${settings:Name}"_el), "second"_el);
        REQUIRE_EQUAL(replacer.replace("${settings:missing}"_el), "${settings:missing}"_el);
    }
};
