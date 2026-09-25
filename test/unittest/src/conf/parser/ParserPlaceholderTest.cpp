// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserTestHelper.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/text/placeholder/Filter.hpp>
#include <erbsland/text/placeholder/ReplacerError.hpp>
#include <erbsland/text/placeholder/Source.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringMap.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Parser Source Filter EnvironmentSource TextFilter)
class ParserPlaceholderTest final : public UNITTEST_SUBCLASS(ParserTestHelper) {
    class EchoSource final : public el::text::placeholder::Source {
    public:
        [[nodiscard]] auto sourceNames() const -> el::text::StringList override {
            return el::text::StringList{"echo"_el, "second source"_el};
        }

        [[nodiscard]] auto resolve(const el::text::String &sourceName, const el::text::String &parameter)
            -> el::text::String override {
            lastSource = sourceName;
            lastParameter = parameter;
            if (parameter == "recursive"_el) {
                return "${echo:inner}"_el;
            }
            if (parameter == "fail"_el) {
                throw el::text::placeholder::ReplacerError{
                    el::text::placeholder::ReplacerErrorCategory::Access, "Injected source failure."_el};
            }
            return parameter;
        }

        el::text::String lastSource;
        el::text::String lastParameter;
    };

    class DecorateFilter final : public el::text::placeholder::Filter {
    public:
        [[nodiscard]] auto filterNames() const -> el::text::StringList override {
            return el::text::StringList{"decorate"_el};
        }

        [[nodiscard]] auto apply(
            const el::text::String &filterName, const el::text::String &parameter, const el::text::String &value)
            -> el::text::String override {
            lastFilter = filterName;
            lastParameter = parameter;
            return el::text::String::fromJoined({"["_el, parameter, ":"_el, value, "]"_el});
        }

        el::text::String lastFilter;
        el::text::String lastParameter;
    };

    class EmptySource final : public el::text::placeholder::Source {
    public:
        [[nodiscard]] auto sourceNames() const -> el::text::StringList override { return {}; }
        [[nodiscard]] auto resolve(const el::text::String &, const el::text::String &) -> el::text::String override {
            return {};
        }
    };

    class CustomVariableSource final : public el::text::placeholder::Source {
    public:
        [[nodiscard]] auto sourceNames() const -> el::text::StringList override {
            return el::text::StringList{"var"_el};
        }
        [[nodiscard]] auto resolve(const el::text::String &, const el::text::String &) -> el::text::String override {
            return "custom"_el;
        }
    };

public:
    [[nodiscard]] auto parseValue(Parser &parser, const el::text::String &valueText) -> el::text::String {
        const auto sourceText = el::text::String::fromJoined({"[main]\nvalue: "_el, valueText, "\n"_el});
        doc = parser.parseTextOrThrow(sourceText);
        return doc->value("main.value"_el)->asText();
    }

    void testDisabledBehavior() {
        auto parser = Parser{};
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:value}")"_el), "${echo:value}"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("\${echo:value}")"_el), "${echo:value}"_el);
        parser.addPlaceholderFilter(std::make_shared<DecorateFilter>());
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:value}")"_el), "${echo:value}"_el);
    }

    void testCustomSourceAndFilter() {
        auto parser = Parser{};
        const auto source = std::make_shared<EchoSource>();
        const auto filter = std::make_shared<DecorateFilter>();
        parser.addPlaceholderSource(source);
        parser.addPlaceholderFilter(filter);
        REQUIRE_EQUAL(
            parseValue(parser, R"("before ${ECHO:Case\u007cPipe|DECORATE:Arg} after")"_el),
            "before [Arg:Case|Pipe] after"_el);
        REQUIRE_EQUAL(source->lastSource, "echo"_el);
        REQUIRE_EQUAL(source->lastParameter, "Case|Pipe"_el);
        REQUIRE_EQUAL(filter->lastFilter, "decorate"_el);
        REQUIRE_EQUAL(filter->lastParameter, "Arg"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:recursive}")"_el), "${echo:inner}"_el);
    }

    void testRegistrationRules() {
        auto parser = Parser{};
        const auto source = std::make_shared<EchoSource>();
        const auto filter = std::make_shared<DecorateFilter>();
        REQUIRE_THROWS_AS(el::err::ParameterError, parser.addPlaceholderSource({}));
        REQUIRE_THROWS_AS(el::err::ParameterError, parser.addPlaceholderFilter({}));
        REQUIRE_THROWS_AS(el::err::LogicError, parser.addPlaceholderSource(std::make_shared<EmptySource>()));
        REQUIRE_NOTHROW(parser.addPlaceholderSource(source));
        REQUIRE_THROWS_AS(el::err::LogicError, parser.addPlaceholderSource(std::make_shared<EchoSource>()));
        REQUIRE_NOTHROW(parser.addPlaceholderFilter(filter));
        REQUIRE_THROWS_AS(el::err::LogicError, parser.addPlaceholderFilter(std::make_shared<DecorateFilter>()));
        parser.removePlaceholderSource(source);
        parser.removePlaceholderFilter(filter);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:value}")"_el), "${echo:value}"_el);
    }

    void testValueScopesAndLists() {
        auto parser = Parser{};
        parser.addPlaceholderSource(std::make_shared<EchoSource>());
        REQUIRE_THROWS_AS(ConfError, parser.parseTextOrThrow("@version: \"${echo:1.0}\"\n"_el));
        doc = parser.parseTextOrThrow(
            "[names]\n"
            "\"${echo:name}\": \"text-name\"\n"
            "[main]\n"
            "code: `${echo:code}`\n"
            "regex: /${echo:regex}/\n"
            "list: \"${echo:one}\", \"${echo:two}\"\n"
            "multi: \"\"\"\n"
            "    before ${echo:inside}\n"
            "    \"\"\"\n"_el);
        REQUIRE_EQUAL(doc->value("names.\"${echo:name}\""_el)->asText(), "text-name"_el);
        REQUIRE_EQUAL(doc->value("main.code"_el)->asText(), "${echo:code}"_el);
        REQUIRE_EQUAL(doc->value("main.list[0]"_el)->asText(), "one"_el);
        REQUIRE_EQUAL(doc->value("main.list[1]"_el)->asText(), "two"_el);
        REQUIRE_EQUAL(doc->value("main.multi"_el)->asText(), "before inside"_el);
    }

    void testSyntaxAndDispatchErrors() {
        auto parser = Parser{};
        parser.addPlaceholderSource(std::make_shared<EchoSource>());
        const auto requireError = [&](const el::text::String &text, const ConfErrorCategory category) {
            try {
                static_cast<void>(parseValue(parser, text));
                REQUIRE(false);
            } catch (const ConfError &error) {
                REQUIRE_EQUAL(error.category(), category);
                REQUIRE(error.context().namePath().has_value());
                REQUIRE_EQUAL(error.context().namePath()->toText(), "main.value"_el);
                REQUIRE(error.context().location().has_value());
                REQUIRE(error.context().codeSnippet().has_value());
            }
        };
        WITH_CONTEXT(requireError(R"("${}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(requireError(R"("${unknown:value}")"_el, ConfErrorCategory::Unsupported));
        WITH_CONTEXT(requireError(R"("${echo:${echo:nested}}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(requireError(R"("${echo:fail}")"_el, ConfErrorCategory::Access));

        auto text = el::text::StringEditor{"\"${echo:value"_el};
        for (auto index = 0U; index < 17U; ++index) {
            text.append("|decorate"_el);
        }
        text.append("}\""_el);
        parser.addPlaceholderFilter(std::make_shared<DecorateFilter>());
        WITH_CONTEXT(requireError(el::text::String{text}, ConfErrorCategory::LimitExceeded));
    }

    void testBuiltInTextFilters() {
        auto parser = Parser{};
        parser.addPlaceholderSource(std::make_shared<EchoSource>());
        parser.addPlaceholderTextFilters();
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:  value  |trim}")"_el), "value"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:abcdef|slice:start=1,length=3}")"_el), "bcd"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:abcdef|slice:2}")"_el), "cdef"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:abcabc|remove:text=ab}")"_el), "cc"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:abcdef|remove:side=back,length=2}")"_el), "abcd"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:abcabc|replace:text=ab,rep=X}")"_el), "XcXc"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:abc|if:contains=b,then=yes,else=no}")"_el), "yes"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:|if:empty,then=yes,else=no}")"_el), "yes"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:abc|error_if:empty}")"_el), "abc"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:a\nb|escape:format=cpp,amount=required}")"_el), "a\\nb"_el);
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${echo:|error_if:empty}")"_el));
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${echo:a|if:empty,then=yes}")"_el));
    }

    void testDefaultRequiredAndCaseFilters() {
        auto parser = Parser{};
        parser.addPlaceholderSource(std::make_shared<EchoSource>());
        parser.addPlaceholderTextFilters();
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:|default:fallback}")"_el), "fallback"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:value|default:fallback}")"_el), "value"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:|default:a,b}")"_el), "a,b"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:value|required}")"_el), "value"_el);
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${echo:|required}")"_el));
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${echo:value|required:extra}")"_el));

        REQUIRE_EQUAL(parseValue(parser, R"("${echo:ÄBC|lower}")"_el), "äbc"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:äbc|upper:unicode}")"_el), "ÄBC"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:ÄBC|lower:ascii}")"_el), "Äbc"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${echo:äbc|upper:ascii}")"_el), "äBC"_el);
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${echo:value|lower:invalid}")"_el));
    }

    void testEnvironmentSource() {
        auto environment = el::system::EnvironmentVariables{};
        const auto variableName = el::text::String{"ERBSLAND_CORE_PLACEHOLDER_TEST"_el};
        const auto previousValue = environment.get(variableName);
        REQUIRE(environment.set(variableName, "a\tb\nc\rd"_el));
        auto parser = Parser{};
        parser.addPlaceholderEnvironmentSource();
        REQUIRE_EQUAL(parseValue(parser, R"("${env:ERBSLAND_CORE_PLACEHOLDER_TEST}")"_el), "a\tb\ncd"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${env:ERBSLAND_CORE_PLACEHOLDER_TEST,unsafe_raw}")"_el), "a\tb\nc\rd"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${env:ERBSLAND_CORE_PLACEHOLDER_MISSING}")"_el), "undefined"_el);
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${env:ERBSLAND_CORE_PLACEHOLDER_MISSING,required}")"_el));
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${env:ERBSLAND_CORE_PLACEHOLDER_TEST,unknown}")"_el));
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${env:,required}")"_el));
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${env:ERBSLAND_CORE_PLACEHOLDER_TEST,}")"_el));
        auto namedParser = Parser{};
        namedParser.addPlaceholderEnvironmentSource("process environment"_el);
        REQUIRE_EQUAL(
            parseValue(namedParser, R"("${PROCESS_ENVIRONMENT:ERBSLAND_CORE_PLACEHOLDER_TEST}")"_el), "a\tb\ncd"_el);
        REQUIRE_THROWS_AS(el::err::LogicError, namedParser.addPlaceholderEnvironmentSource("Process_Environment"_el));
        if (previousValue.has_value()) {
            REQUIRE(environment.set(variableName, *previousValue));
        } else {
            REQUIRE(environment.remove(variableName));
        }
    }

    void testVariableSource() {
        auto parser = Parser{};
        parser.setPlaceholderVariableSource(
            el::text::StringMap<el::text::String>{{
                {"Project Name"_el, "Erbsland Core"_el},
                {"empty"_el, ""_el},
            }});
        REQUIRE_EQUAL(parseValue(parser, R"("${var:project_name}")"_el), "Erbsland Core"_el);
        REQUIRE_EQUAL(parseValue(parser, R"("${VAR:PROJECT NAME}")"_el), "Erbsland Core"_el);
        REQUIRE(parseValue(parser, R"("${var:empty}")"_el).isEmpty());
        REQUIRE_THROWS_AS(ConfError, parseValue(parser, R"("${var:missing}")"_el));

        parser.setPlaceholderVariableSource(el::text::StringMap<el::text::String>{{{"project_name"_el, "Updated"_el}}});
        REQUIRE_EQUAL(parseValue(parser, R"("${var:PROJECT NAME}")"_el), "Updated"_el);
        REQUIRE_THROWS_AS(
            ConfError,
            parser.setPlaceholderVariableSource(
                el::text::StringMap<el::text::String>{{
                    {"duplicate name"_el, "first"_el},
                    {"Duplicate_Name"_el, "second"_el},
                }}));

        auto parserWithCustomVariableSource = Parser{};
        parserWithCustomVariableSource.addPlaceholderSource(std::make_shared<CustomVariableSource>());
        REQUIRE_THROWS_AS(
            el::err::LogicError,
            parserWithCustomVariableSource.setPlaceholderVariableSource(
                el::text::StringMap<el::text::String>{{{"name"_el, "value"_el}}}));

        auto namedParser = Parser{};
        namedParser.setPlaceholderVariableSource(
            el::text::StringMap<el::text::String>{{{"survey area"_el, "coast"_el}}}, "survey values"_el);
        REQUIRE_EQUAL(parseValue(namedParser, R"("${SURVEY_VALUES:survey_area}")"_el), "coast"_el);
        namedParser.setPlaceholderVariableSource(
            el::text::StringMap<el::text::String>{{{"survey area"_el, "reef"_el}}}, "Survey_Values"_el);
        REQUIRE_EQUAL(parseValue(namedParser, R"("${survey values:survey area}")"_el), "reef"_el);
        REQUIRE_THROWS_AS(
            ConfError,
            namedParser.setPlaceholderVariableSource(
                el::text::StringMap<el::text::String>{{{"area"_el, "reef"_el}}}, "invalid-name"_el));
    }
};
