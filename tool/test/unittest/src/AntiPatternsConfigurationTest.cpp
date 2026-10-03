// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/anti_patterns/AntiPatternsTool.hpp>
#include <tool/anti_patterns/Policy.hpp>
#include <tool/anti_patterns/RuleRegistry.hpp>
#include <tool/anti_patterns/Scanner.hpp>
#include <tool/anti_patterns/SourceIndex.hpp>

TESTED_TARGETS(
    RuleRegistry NamespaceNoiseRule NamedConstantForRepeatedLiteralsRule MissingApiDocumentationRule FindingCache)
class AntiPatternsConfigurationTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
private:
    static constexpr auto cAntiPatternsKey = "anti_patterns"_el;

public: // implement UnitTest
    void setUp() override {
        ProjectFixture::setUp();
        _rules = tool::anti_patterns::RuleRegistry{};
    }

public: // tests
    void testExceptionPathLists() {
        const auto service = files();
        auto policy = tool::anti_patterns::Policy{};
        for (
            const auto &paths : el::StringList{
                "\" src/First.cpp \""_el,
                "\" src/First.cpp \", \"src/nested/**\""_el,
                "\n    * \" src/First.cpp \"\n    * \"src/nested/**\""_el}) {
            _failureDiagnostic = paths;
            const auto configuration = parse(
                el::join(
                    "*[anti_patterns.exception]\nPath: "_el,
                    paths,
                    "\nRules: \"anonymous_namespace\", \"static_cast_void\"\nReason: \" Shared reason. \"\n"_el));
            policy.read(configuration, service, _rules);
            for (const auto &rule : el::StringList{"anonymous_namespace"_el, "static_cast_void"_el}) {
                REQUIRE_EQUAL(policy.reason(el::Path{"src/First.cpp"_el}, rule), "Shared reason."_el);
                if (paths.contains("nested"_el)) {
                    REQUIRE_EQUAL(policy.reason(el::Path{"src/nested/deeper/Other.cpp"_el}, rule), "Shared reason."_el);
                } else {
                    REQUIRE(policy.reason(el::Path{"src/nested/deeper/Other.cpp"_el}, rule).isEmpty());
                }
                REQUIRE(policy.reason(el::Path{"src/Unlisted.cpp"_el}, rule).isEmpty());
            }
            REQUIRE(policy.reason(el::Path{"src/First.cpp"_el}, "regular_string_literal"_el).isEmpty());
        }
    }

    void testInvalidExceptionPathLists() {
        const auto service = files();
        auto policy = tool::anti_patterns::Policy{};
        for (
            const auto &paths : el::StringList{
                "12"_el,
                "\"src/First.cpp\", 12"_el,
                "\"\""_el,
                "\"src/First.cpp\", \"  \""_el,
                "\"src/First.cpp\", \"/outside/*\""_el,
                "\"src/First.cpp\", \"../outside/*\""_el,
                "\"src/First.cpp\", \"src/[\""_el,
                "\"src/First.cpp\", \" src/First.cpp \""_el}) {
            _failureDiagnostic = paths;
            REQUIRE_THROWS(policy.read(
                parse(
                    el::join(
                        "*[anti_patterns.exception]\nPath: "_el,
                        paths,
                        "\nRules: \"anonymous_namespace\"\nReason: \"Shared reason.\"\n"_el)),
                service,
                _rules));
        }
        REQUIRE_THROWS(policy.read(
            parse(
                "*[anti_patterns.exception]\nPath: \"src/First.cpp\", \"src/Second.cpp\"\n"
                "Rules: \"anonymous_namespace\"\nReason: \"First reason.\"\n"
                "*[anti_patterns.exception]\nPath: \"src/Third.cpp\", \"src/Second.cpp\"\n"
                "Rules: \"static_cast_void\", \"anonymous_namespace\"\nReason: \"Second reason.\"\n"_el),
            service,
            _rules));
    }

    void testCachedFindingsUseEveryExceptionPath() {
        write("src/First.cpp"_el, "namespace {\n}\n"_el);
        write("src/nested/Second.cpp"_el, "namespace {\n}\n"_el);
        write("src/Unlisted.cpp"_el, "namespace {\n}\n"_el);
        const auto service = files();
        auto policy = tool::anti_patterns::Policy{};
        policy.read(parse(), service, _rules);
        auto first = tool::anti_patterns::Scanner{policy, _rules, service};
        REQUIRE_EQUAL(first.scan({}, 1).count(), el::ItemCount{3});
        policy.read(
            parse(
                "*[anti_patterns.exception]\nPath: \"src/First.cpp\", \"src/nested/**\"\n"
                "Rules: \"anonymous_namespace\"\nReason: \"Shared reason.\"\n"_el),
            service,
            _rules);
        auto cached = tool::anti_patterns::Scanner{policy, _rules, service};
        const auto findings = cached.scan({}, 4);
        REQUIRE_EQUAL(cached.cacheHits(), el::ItemCount{3});
        REQUIRE_EQUAL(findings.count(), el::ItemCount{3});
        for (const auto &finding : findings) {
            if (finding.path == el::Path{"src/Unlisted.cpp"_el}) {
                REQUIRE_FALSE(finding.isSuppressed());
            } else {
                REQUIRE(finding.isSuppressed());
                REQUIRE_EQUAL(finding.configReason, "Shared reason."_el);
            }
        }
    }

    void testOptionalSettingsAndReplacement() {
        const auto text = "namespace sample { mem::A a; mem::B b; mem::C c; mem::D d; }"_el;
        REQUIRE(candidates("namespace_noise"_el, text).isEmpty());
        configure("[anti_patterns.namespace_noise]\nDomains: \"mem\"\n"_el);
        REQUIRE_EQUAL(candidates("namespace_noise"_el, text).count(), el::ItemCount{1});
        configure("[anti_patterns.namespace_noise]\nDomains: \"custom\"\n"_el);
        REQUIRE(candidates("namespace_noise"_el, text).isEmpty());
        REQUIRE_EQUAL(
            candidates(
                "namespace_noise"_el, "namespace sample { custom::A a; custom::B b; custom::C c; custom::D d; }"_el)
                .count(),
            el::ItemCount{1});
        configure();
        REQUIRE(candidates("namespace_noise"_el, text).isEmpty());
        configure("[anti_patterns.namespace_noise]\n"_el);
        REQUIRE(candidates("namespace_noise"_el, text).isEmpty());
    }

    void testIgnoredSymbolsAndFirstCountedLocation() {
        configure(
            "[anti_patterns.namespace_noise]\nDomains: \"mem\", \"text\"\n"
            "Ignored Symbols: \"mem::secureErase\", \"mem::nested::erase\"\n"_el);
        const auto ignored = "mem::secureErase(a); el::mem::secureErase(b); erbsland::mem::secureErase(c); "
                             "mem /* qualification */ :: secureErase<int>(d); mem::nested::erase(e); "
                             "auto pointer = &mem::secureErase; "_el;
        REQUIRE(candidates("namespace_noise"_el, el::join("namespace sample { "_el, ignored, " }"_el)).isEmpty());
        const auto text = el::join("namespace sample { "_el, ignored, "mem::A a; mem::B b; mem::C c; mem::D d; }"_el);
        const auto found = candidates("namespace_noise"_el, text);
        REQUIRE_EQUAL(found.count(), el::ItemCount{1});
        REQUIRE_EQUAL(found.first().beginIndex(), text.find("mem::A"_el));
        REQUIRE_EQUAL(
            candidates(
                "namespace_noise"_el,
                "namespace sample { mem::secureEraseOther(a); mem::SecureErase(b); "
                "mem::nested::eraseOther(c); mem::secureErase::Other d; }"_el)
                .count(),
            el::ItemCount{1});
        REQUIRE(candidates(
            "namespace_noise"_el, "namespace sample { mem::secureErase(a); mem::A a; mem::B b; mem::C c; }"_el)
                .isEmpty());
        REQUIRE_EQUAL(
            candidates(
                "namespace_noise"_el,
                "namespace sample { mem::secureErase(a); text::A a; text::B b; text::C c; text::D d; }"_el)
                .count(),
            el::ItemCount{1});
        configure("[anti_patterns.namespace_noise]\nDomains: \"mem\"\n"_el);
        REQUIRE_EQUAL(
            candidates("namespace_noise"_el, el::join("namespace sample { "_el, ignored, " }"_el)).count(),
            el::ItemCount{1});
    }

    void testAccessorNamesPrefixesAndDemos() {
        const auto exact = "config.lookup(\"key\"_el); config.lookup(\"key\"_el);"_el;
        const auto prefixed = "config.readText(\"key\"_el); config.readInteger(\"key\"_el);"_el;
        REQUIRE(candidates("named_constant_for_repeated_literals"_el, exact).isEmpty());
        configure(
            "[anti_patterns.named_constant_for_repeated_literals]\n"
            "Key Functions: \"lookup\"\nKey Function Prefixes: \"read\"\n"_el);
        REQUIRE_EQUAL(candidates("named_constant_for_repeated_literals"_el, exact).count(), el::ItemCount{1});
        REQUIRE_EQUAL(candidates("named_constant_for_repeated_literals"_el, prefixed).count(), el::ItemCount{1});
        REQUIRE(candidates(
            "named_constant_for_repeated_literals"_el,
            "config.lookupOther(\"key\"_el); config.lookupOther(\"key\"_el);"_el)
                .isEmpty());
        REQUIRE(candidates(
            "named_constant_for_repeated_literals"_el, "config.Lookup(\"key\"_el); config.Lookup(\"key\"_el);"_el)
                .isEmpty());
        REQUIRE_EQUAL(
            candidates("named_constant_for_repeated_literals"_el, exact, "demos/Example.cpp"_el).count(),
            el::ItemCount{1});
        configure();
        REQUIRE(candidates("named_constant_for_repeated_literals"_el, exact).isEmpty());
    }

    void testApiDocumentationScopes() {
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"src/Example.hpp"_el}, R"cpp(
namespace example {
void freeFunction();
/// Documented outer type.
class Outer {
public:
    /// Documented nested type.
    class Inner {
    public:
        Inner();
        ~Inner();
    };
    /// Documented implementation.
    void implementation() {
        class Local {
        public:
            Local();
            void helper();
        };
    }
};
}
)cpp"_el};
        auto candidates = el::List<el::ByteRange>{};
        _rules.find("missing_api_documentation"_el)->scan(source, candidates);
        REQUIRE_EQUAL(candidates.count(), el::ItemCount{3});
        REQUIRE_EQUAL(source.slice(candidates.getRef(el::ItemIndex{0})), "void freeFunction();"_el);
        REQUIRE_EQUAL(source.slice(candidates.getRef(el::ItemIndex{1})), "Inner();"_el);
        REQUIRE_EQUAL(source.slice(candidates.getRef(el::ItemIndex{2})), "~Inner();"_el);
    }

    void testDocumentationExemptionScopes() {
        const auto file = "class Suite : public CustomTest {\npublic:\nvoid run();\n};\nvoid other();\n"_el;
        REQUIRE_FALSE(candidates("missing_api_documentation"_el, file, "src/Example.hpp"_el).isEmpty());
        configure(
            "[anti_patterns.missing_api_documentation]\nFile Exemption Texts: \"CustomTest\"\n"
            "Declaration Exemption Texts: \"LocalTag\"\n"_el);
        REQUIRE(candidates("missing_api_documentation"_el, file, "src/Example.hpp"_el).isEmpty());
        const auto declarations = el::String{"void hidden(LocalTag);\nvoid visible();\n"_el};
        const auto found = candidates("missing_api_documentation"_el, declarations, "src/Example.hpp"_el);
        REQUIRE_EQUAL(found.count(), el::ItemCount{1});
        REQUIRE_EQUAL(found.first().beginIndex(), declarations.find("void visible"_el));
        REQUIRE_FALSE(
            candidates("missing_api_documentation"_el, "// CustomTest\nclass Example {};\n"_el, "src/Example.hpp"_el)
                .isEmpty());
        REQUIRE_FALSE(candidates(
            "missing_api_documentation"_el,
            "class Example {\nauto text() { return \"CustomTest\"; }\n};\n"_el,
            "src/Example.hpp"_el)
                .isEmpty());
        REQUIRE_FALSE(
            candidates("missing_api_documentation"_el, "class Suite : public customtest {};\n"_el, "src/Example.hpp"_el)
                .isEmpty());
        configure();
        REQUIRE_FALSE(candidates("missing_api_documentation"_el, file, "src/Example.hpp"_el).isEmpty());
        REQUIRE_EQUAL(
            candidates("missing_api_documentation"_el, declarations, "src/Example.hpp"_el).count(), el::ItemCount{2});
    }

    void testInvalidSettings() {
        for (
            const auto &settings : el::StringList{
                "[anti_patterns.unknown_rule]\n"_el,
                "[anti_patterns.namespace_noise]\nUnknown: \"value\"\n"_el,
                "[anti_patterns.namespace_noise]\nDomains: 12\n"_el,
                "[anti_patterns.namespace_noise]\nDomains: \"mem\", 12\n"_el,
                "[anti_patterns.namespace_noise]\nDomains: \"mem::nested\"\n"_el,
                "[anti_patterns.namespace_noise]\nIgnored Symbols: \"mem::*\"\n"_el,
                "[anti_patterns.namespace_noise]\nIgnored Symbols: \"secureErase\"\n"_el,
                "[anti_patterns.namespace_noise]\nIgnored Symbols: \"mem::\"\n"_el,
                "[anti_patterns.named_constant_for_repeated_literals]\nKey Functions: \"\"\n"_el,
                "[anti_patterns.named_constant_for_repeated_literals]\nKey Function Prefixes: \"bad name\"\n"_el,
                "[anti_patterns.missing_api_documentation]\nFile Exemption Texts: \"  \"\n"_el,
                "[anti_patterns.missing_api_documentation]\nDeclaration Exemption Texts: true\n"_el,
                "[anti_patterns.anonymous_namespace]\nUnknown: \"value\"\n"_el}) {
            _failureDiagnostic = settings;
            REQUIRE_THROWS(parse(settings));
        }
    }

    void testCacheSettingsAndConfiguredDemoException() {
        write("src/Example.cpp"_el, "namespace sample { mem::A a; mem::B b; mem::C c; mem::D d; }\n"_el);
        const auto service = files();
        const auto configuration = parse("[anti_patterns.namespace_noise]\nDomains: \"mem\"\n"_el);
        auto policy = tool::anti_patterns::Policy{};
        policy.read(configuration, service, _rules);
        auto first = tool::anti_patterns::Scanner{policy, _rules, service};
        REQUIRE(first.scan({}, 1).isEmpty());
        _rules.configure(configuration);
        auto changed = tool::anti_patterns::Scanner{policy, _rules, service};
        REQUIRE_EQUAL(changed.scan({}, 1).count(), el::ItemCount{1});
        REQUIRE_EQUAL(changed.cacheMisses(), el::ItemCount{1});
        const auto signature = _rules.configurationSignature();
        _rules.configure(
            parse("[anti_patterns.namespace_noise]\n# Formatting does not affect identity.\nDomains: \"mem\"\n"_el));
        REQUIRE_EQUAL(_rules.configurationSignature(), signature);
        auto same = tool::anti_patterns::Scanner{policy, _rules, service};
        REQUIRE_EQUAL(same.scan({}, 4).count(), el::ItemCount{1});
        REQUIRE_EQUAL(same.cacheHits(), el::ItemCount{1});
        configure(
            "[anti_patterns.namespace_noise]\nDomains: \"mem\"\n"
            "Ignored Symbols: \"mem::A\"\n"_el);
        auto ignored = tool::anti_patterns::Scanner{policy, _rules, service};
        REQUIRE(ignored.scan({}, 1).isEmpty());
        REQUIRE_EQUAL(ignored.cacheMisses(), el::ItemCount{1});
        configure();
        auto reset = tool::anti_patterns::Scanner{policy, _rules, service};
        REQUIRE(reset.scan({}, 1).isEmpty());
        REQUIRE_EQUAL(reset.cacheMisses(), el::ItemCount{1});

        write("demos/Example.cpp"_el, "config.lookup(\"key\"_el); config.lookup(\"key\"_el);\n"_el);
        const auto demo = parse(
            "[anti_patterns.named_constant_for_repeated_literals]\nKey Functions: \"lookup\"\n"
            "*[anti_patterns.exception]\nPath: \"demos/**\"\nRules: \"named_constant_for_repeated_literals\"\n"
            "Reason: \"Keep demonstration keys inline.\"\n"_el);
        _rules.configure(demo);
        policy.read(demo, service, _rules);
        auto scanner = tool::anti_patterns::Scanner{policy, _rules, service};
        const auto findings = scanner.scan(el::StringList{"demos/Example.cpp"_el}, 1);
        REQUIRE_EQUAL(findings.count(), el::ItemCount{1});
        REQUIRE(findings.first().isSuppressed());
    }

    void testCacheDigestIncludesPolicyExceptionsAndConfiguredOrder() {
        REQUIRE(_rules.configurationSignature().isEmpty());
        configure();
        const auto original = _rules.configurationSignature();
        REQUIRE_FALSE(original.isEmpty());
        const auto changedPolicy =
            el::conf::Parser{}
                .parseTextOrThrow(
                    "[anti_patterns]\nSource Directories: \"include\"\nFile Suffixes: \".cpp\", \".hpp\"\n"_el)
                ->valueOrThrow(cAntiPatternsKey);
        tool::anti_patterns::AntiPatternsTool{}.configurationRules()->validate(changedPolicy, 1);
        _rules.configure(changedPolicy);
        REQUIRE_NOT_EQUAL(_rules.configurationSignature(), original);
        configure(
            "*[anti_patterns.exception]*\npath: \"src/**\"\nrules: \"anonymous_namespace\"\nreason: \"Fixture\"\n"_el);
        REQUIRE_NOT_EQUAL(_rules.configurationSignature(), original);
        configure("[anti_patterns.namespace_noise]\nDomains: \"mem\"\nIgnored Symbols: \"mem::Item\"\n"_el);
        const auto settings = _rules.configurationSignature();
        configure("[anti_patterns.namespace_noise]\nIgnored Symbols: \"mem::Item\"\nDomains: \"mem\"\n"_el);
        REQUIRE_NOT_EQUAL(_rules.configurationSignature(), settings);
        configure(
            "[anti_patterns.namespace_noise]\nDomains: \"mem\"\n[anti_patterns.named_constant_for_repeated_literals]\nKey Functions: \"read\"\n"_el);
        const auto sections = _rules.configurationSignature();
        configure(
            "[anti_patterns.named_constant_for_repeated_literals]\nKey Functions: \"read\"\n[anti_patterns.namespace_noise]\nDomains: \"mem\"\n"_el);
        REQUIRE_NOT_EQUAL(_rules.configurationSignature(), sections);
    }

private:
    [[nodiscard]] auto parse(const el::String &settings = {}) -> el::conf::ValuePtr {
        const auto parsed = el::conf::Parser{}.parseTextOrThrow(
            el::join("[Anti Patterns]\nSource Directories: \"src\"\nFile Suffixes: \".cpp\", \".hpp\"\n"_el, settings));
        const auto section = parsed->valueOrThrow(cAntiPatternsKey);
        tool::anti_patterns::AntiPatternsTool{}.configurationRules()->validate(section, 1);
        return section;
    }

    void configure(const el::String &settings = {}) { _rules.configure(parse(settings)); }

    [[nodiscard]] auto candidates(
        const el::String &rule, const el::String &text, const el::String &path = "src/Example.cpp"_el)
        -> el::List<el::ByteRange> {
        _failureDiagnostic = text;
        auto result = el::List<el::ByteRange>{};
        _rules.find(rule)->scan(tool::anti_patterns::SourceIndex{el::Path{path}, text}, result);
        return result;
    }

private:
    tool::anti_patterns::RuleRegistry _rules; ///< Detectors configured by each scenario.
};
