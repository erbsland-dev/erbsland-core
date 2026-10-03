// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/anti_patterns/AntiPatternsTool.hpp>
#include <tool/anti_patterns/FindingCache.hpp>
#include <tool/anti_patterns/Policy.hpp>
#include <tool/anti_patterns/RuleRegistry.hpp>
#include <tool/anti_patterns/Scanner.hpp>
#include <tool/anti_patterns/SourceIndex.hpp>

#include <algorithm>
#include <array>
#include <tuple>

TESTED_TARGETS(
    tool::anti_patterns::SourceIndex tool::anti_patterns::Policy tool::anti_patterns::Scanner
        tool::anti_patterns::FindingCache)
class AntiPatternsTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
    constexpr static auto cAntiPatternsKey = "anti_patterns"_el;

public:
    void testEveryImplementedRule() {
        using Case = std::tuple<el::String, el::String, el::String>;
        auto oversizedText = el::StringEditor{};
        for (auto index = el::LineIndex::zero(); index.isWithin(el::LineCount{501}); ++index) {
            oversizedText.append("// line\n"_el);
        }
        const auto cases = std::array<Case, 23>{
            Case{"unnecessary_unit_conversion"_el, "src/Example.cpp"_el, "x.toSizeT() >= 16U;"_el},
            Case{"unnecessary_type_construction"_el, "src/Example.cpp"_el, "CharSet{Char{U'x'}};"_el},
            Case{
                "namespace_noise"_el,
                "src/Example.cpp"_el,
                "namespace a { text::A a; text::B b; text::C c; text::D d; }"_el},
            Case{"oversized_file"_el, "src/Example.cpp"_el, el::String{oversizedText}},
            Case{"anonymous_namespace"_el, "src/Example.cpp"_el, "namespace {\n}\n"_el},
            Case{"type_in_wrong_unit"_el, "src/Example.cpp"_el, "class Example {\n};\n"_el},
            Case{
                "namespace_in_wrong_unit"_el,
                "src/erbsland/example/Example.hpp"_el,
                "namespace erbsland::other {\n}\n"_el},
            Case{
                "implementation_in_wrong_unit"_el,
                "src/Other.cpp"_el,
                "auto Example::value() const -> int { return 1; }\n"_el},
            Case{
                "oversized_nested_type"_el,
                "src/Example.hpp"_el,
                "class Outer {\nclass Inner {\nint a;\nint b;\nint c;\nint d;\nint e;\nint f;\nint g;\nint h;\nint i;\nint j;\nint k;\n};\n};\n"_el},
            Case{"forward_declaration"_el, "src/Example.hpp"_el, "class Other;\n"_el},
            Case{"static_global_object"_el, "src/Example.cpp"_el, "Widget globalWidget{1};\n"_el},
            Case{
                "static_only_class"_el,
                "src/Example.hpp"_el,
                "class Helper { public: static void run(); static int value(); };\n"_el},
            Case{"missing_api_documentation"_el, "src/Example.hpp"_el, "class Example {\n};\n"_el},
            Case{
                "api_documentation_on_member_implementation"_el,
                "src/Example.hpp"_el,
                "/// Incorrect location.\nauto Example::value() -> int { return 1; }\n"_el},
            Case{
                "missing_default_group_comment"_el,
                "src/Example.hpp"_el,
                "class Example {\npublic:\nExample() = default;\n};\n"_el},
            Case{"multiple_types_in_header"_el, "src/Example.hpp"_el, "class Example {};\nclass Other {};\n"_el},
            Case{"regular_string_literal"_el, "src/Example.cpp"_el, "auto value = \"plain\";\n"_el},
            Case{
                "misplaced_unittest_metadata"_el,
                "src/Example.cpp"_el,
                "void probe() {\nTAGS(FullRun)\nconst auto value = 1;\n}\n"_el},
            Case{"static_cast_void"_el, "src/Example.cpp"_el, "static_cast<void>(operation());\n"_el},
            Case{"nested_namespace"_el, "src/Example.cpp"_el, "namespace outer {\nnamespace inner {\n}\n}\n"_el},
            Case{"multiple_namespaces"_el, "src/Example.cpp"_el, "namespace a {} namespace b {}"_el},
            Case{"erbsland_core_containers_strict"_el, "src/Example.cpp"_el, "std::vector<el::String> names;\n"_el},
            Case{
                "named_constant_for_repeated_literals"_el,
                "src/Example.cpp"_el,
                "auto first = config.getTextOrThrow(\"key\"_el);\n"
                "auto second = config.getTextOrThrow(\"key\"_el);\n"_el},
        };
        auto registry = tool::anti_patterns::RuleRegistry{};
        const auto configuration = el::conf::Parser{}.parseTextOrThrow(
            "[anti_patterns.namespace_noise]\nDomains: \"text\"\n"
            "[anti_patterns.named_constant_for_repeated_literals]\nKey Function Prefixes: \"getText\"\n"_el);
        registry.configure(configuration->valueOrThrow(cAntiPatternsKey));
        for (const auto &[identifier, path, text] : cases) {
            _failureDiagnostic = identifier;
            const auto *rule = registry.find(identifier);
            REQUIRE(rule != nullptr);
            const auto source = tool::anti_patterns::SourceIndex{el::Path{path}, text};
            auto candidates = el::List<el::ByteRange>{};
            rule->scan(source, candidates);
            REQUIRE_FALSE(candidates.isEmpty());
        }
        REQUIRE(registry.find("unnecessary_conversion"_el) == nullptr);
    }

    void testLexicalEdges() {
        const auto text = "#define TEXT \"namespace {\"\n// namespace {\n"
                          "auto text = R\"tag(namespace {)tag\";\nnamespace {\n}\n"_el;
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"src/Example.cpp"_el}, text};
        const auto registry = tool::anti_patterns::RuleRegistry{};
        auto candidates = el::List<el::ByteRange>{};
        registry.find("anonymous_namespace"_el)->scan(source, candidates);
        REQUIRE_EQUAL(candidates.count(), el::ItemCount{1});
        REQUIRE_EQUAL(source.literals().count(), el::ItemCount{1});
        const auto conditional = tool::anti_patterns::SourceIndex{
            el::Path{"src/Example.cpp"_el},
            "call();\n#ifdef ERBSLAND_OS_WINDOWS\ncall();\n#else\ncall();\n#endif\n"_el};
        REQUIRE_FALSE(conditional.insideOsConditional(el::ByteIndex{0}));
        REQUIRE(conditional.insideOsConditional(el::ByteIndex{45}));
        const auto nested = tool::anti_patterns::SourceIndex{
            el::Path{"src/Example.hpp"_el}, "namespace sample { class Outer { class Inner {}; }; }\n"_el};
        REQUIRE_FALSE(nested.namespaceScope(el::ByteIndex{40}));
    }

    void testRuleExclusions() {
        using Case = std::tuple<el::String, el::String, el::String>;
        const auto cases = std::array<Case, 20>{
            Case{"oversized_file"_el, "src/Example.cpp"_el, "// short\n"_el},
            Case{"anonymous_namespace"_el, "src/Example.cpp"_el, "// namespace {\n"_el},
            Case{
                "type_in_wrong_unit"_el,
                "src/erbsland/example/Example.hpp"_el,
                "namespace erbsland::example { class Example {}; }\n"_el},
            Case{
                "namespace_in_wrong_unit"_el,
                "src/erbsland/example/Example.hpp"_el,
                "namespace erbsland::example { }\n"_el},
            Case{
                "implementation_in_wrong_unit"_el,
                "src/Example.cpp"_el,
                "auto Example::value() -> int { return 1; }\n"_el},
            Case{"oversized_nested_type"_el, "src/Example.hpp"_el, "class Outer { class Inner { int value; }; };\n"_el},
            Case{"forward_declaration"_el, "src/Example.hpp"_el, "// class Other;\n"_el},
            Case{"static_global_object"_el, "src/Example.cpp"_el, "constexpr Widget globalWidget{1};\n"_el},
            Case{
                "static_only_class"_el,
                "src/Example.hpp"_el,
                "class Helper { public: void run(); static int value(); };\n"_el},
            Case{"missing_api_documentation"_el, "src/Example.hpp"_el, "/// Documented type.\nclass Example {};\n"_el},
            Case{
                "api_documentation_on_member_implementation"_el,
                "src/Example.hpp"_el,
                "auto Example::value() -> int { return 1; }\n"_el},
            Case{
                "missing_default_group_comment"_el,
                "src/Example.hpp"_el,
                "class Example {\npublic:\n/// Default constructor.\nExample() = default;\n};\n"_el},
            Case{"multiple_types_in_header"_el, "src/Example.hpp"_el, "class Example {};\n"_el},
            Case{"regular_string_literal"_el, "src/Example.cpp"_el, "auto value = \"core\"_el;\n"_el},
            Case{"misplaced_unittest_metadata"_el, "src/Example.cpp"_el, "TAGS(FullRun)\nclass Example {};\n"_el},
            Case{"static_cast_void"_el, "src/Example.cpp"_el, "// static_cast<void>(operation());\n"_el},
            Case{"nested_namespace"_el, "src/Example.cpp"_el, "namespace outer { }\n"_el},
            Case{"multiple_namespaces"_el, "src/Example.cpp"_el, "namespace a {} namespace a {}"_el},
            Case{"erbsland_core_containers_strict"_el, "src/Example.cpp"_el, "// std::vector<el::String> names;\n"_el},
            Case{
                "named_constant_for_repeated_literals"_el,
                "src/Example.cpp"_el,
                "auto first = config.getTextOrThrow(\"first\"_el);\n"
                "auto second = config.getTextOrThrow(\"second\"_el);\n"_el},
        };
        auto registry = tool::anti_patterns::RuleRegistry{};
        const auto configuration = el::conf::Parser{}.parseTextOrThrow(
            "[anti_patterns.namespace_noise]\nDomains: \"text\"\n"
            "[anti_patterns.named_constant_for_repeated_literals]\nKey Function Prefixes: \"getText\"\n"_el);
        registry.configure(configuration->valueOrThrow(cAntiPatternsKey));
        for (const auto &[identifier, path, text] : cases) {
            _failureDiagnostic = identifier;
            const auto source = tool::anti_patterns::SourceIndex{el::Path{path}, text};
            auto candidates = el::List<el::ByteRange>{};
            registry.find(identifier)->scan(source, candidates);
            REQUIRE(candidates.isEmpty());
        }
    }

    void testStaticOnlyInlineMembers() {
        using Case = std::pair<el::String, bool>;
        const auto cases = std::array{
            Case{"struct Helper { static auto value() -> int { return 1; } };"_el, true},
            Case{
                "struct Helper { template <math::NativeInteger Value, math::NativeInteger T> "
                "[[nodiscard]] static constexpr auto checked(T value, Value minimum, Value maximum) -> Value { "
                "if (value < minimum) { fail(); } return static_cast<Value>(value); } };"_el,
                true},
            Case{"class Helper { public: static void run() {} static int value(); };"_el, true},
            Case{"struct Helper { static constexpr int cValue{1}; };"_el, true},
            Case{"struct Helper { static void run() {} int value; };"_el, false},
            Case{"struct Helper { static void run() {} int value{}; };"_el, false},
            Case{"struct Helper { static void run() {} auto value() const -> int { return 1; } };"_el, false},
            Case{"struct Helper { static void run() {} virtual ~Helper() = default; };"_el, false},
            Case{"struct Helper : Base { static void run() {} };"_el, false},
        };
        const auto registry = tool::anti_patterns::RuleRegistry{};
        for (const auto &[text, shouldReport] : cases) {
            _failureDiagnostic = text;
            const auto source = tool::anti_patterns::SourceIndex{el::Path{"src/Helper.hpp"_el}, text};
            auto candidates = el::List<el::ByteRange>{};
            registry.find("static_only_class"_el)->scan(source, candidates);
            REQUIRE_EQUAL(!candidates.isEmpty(), shouldReport);
        }
    }

    void testOsConditionalFindingsAreExcluded() {
        write("src/Platform.cpp"_el, "#ifdef ERBSLAND_OS_WINDOWS\nnamespace {\n}\n#else\nnamespace {\n}\n#endif\n"_el);
        const auto policy = makePolicy();
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto service = files();
        auto scanner = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE(scanner.scan({}, 2).isEmpty());
    }

    void testInlineConfiguredExceptionsAndWorkers() {
        write(
            "src/Inline.cpp"_el,
            "// anti-pattern: allow anonymous_namespace -- Local linkage is intentional.\nnamespace {\n}\n"_el);
        write("src/Configured.cpp"_el, "namespace {\n}\n"_el);
        auto policy = makePolicy(
            "--*[ Anti Patterns . Exception ]*--\nPath: \"src/Configured.cpp\"\n"
            "Rules: \"anonymous_namespace\"\nReason: \"Local linkage is intentional.\"\n"_el);
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto service = files();
        auto first = tool::anti_patterns::Scanner{policy, registry, service};
        const auto firstFindings = first.scan({}, 1);
        REQUIRE_EQUAL(first.cacheMisses(), el::ItemCount{2});
        REQUIRE_EQUAL(firstFindings.count(), el::ItemCount{2});
        for (const auto &finding : firstFindings) {
            REQUIRE(finding.isSuppressed());
        }
        auto second = tool::anti_patterns::Scanner{policy, registry, service};
        const auto secondFindings = second.scan({}, 4);
        REQUIRE_EQUAL(second.cacheHits(), el::ItemCount{2});
        REQUIRE_EQUAL(secondFindings.count(), firstFindings.count());
        for (auto i = el::ItemIndex::zero(); i.isWithin(firstFindings.count()); ++i) {
            REQUIRE_EQUAL(secondFindings.getRef(i).rule, firstFindings.getRef(i).rule);
            REQUIRE_EQUAL(secondFindings.getRef(i).path, firstFindings.getRef(i).path);
            REQUIRE_EQUAL(secondFindings.getRef(i).range, firstFindings.getRef(i).range);
            REQUIRE_EQUAL(secondFindings.getRef(i).lines, firstFindings.getRef(i).lines);
            REQUIRE_EQUAL(secondFindings.getRef(i).snippet, firstFindings.getRef(i).snippet);
            REQUIRE_EQUAL(secondFindings.getRef(i).inlineReason, firstFindings.getRef(i).inlineReason);
        }
        policy = makePolicy();
        auto changedPolicy = tool::anti_patterns::Scanner{policy, registry, service};
        const auto changedFindings = changedPolicy.scan({}, 1);
        REQUIRE_EQUAL(changedPolicy.cacheHits(), el::ItemCount{2});
        REQUIRE_EQUAL(
            changedFindings.countIf(
                [](const tool::anti_patterns::Finding &finding) -> bool { return finding.isSuppressed(); }),
            el::ItemCount{1});
        write("src/Configured.cpp"_el, "namespace {\n}\nnamespace {\n}\n"_el);
        auto modified = tool::anti_patterns::Scanner{policy, registry, service};
        const auto modifiedFindings = modified.scan({}, 4);
        REQUIRE_EQUAL(modified.cacheMisses(), el::ItemCount{1});
        REQUIRE_EQUAL(modifiedFindings.count(), el::ItemCount{3});
    }

    void testUnicodeFindingsAcrossCacheAndWorkers() {
        const auto text = el::String{"// ä🙂\r\nnamespace /* before\0after */ { }\r\n"_el};
        write("src/First.cpp"_el, text);
        write("src/Second.cpp"_el, text);
        const auto policy = makePolicy();
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto service = files();
        auto serial = tool::anti_patterns::Scanner{policy, registry, service};
        const auto expected = serial.scan({}, 1);
        REQUIRE_EQUAL(expected.count(), el::ItemCount{2});
        const auto range = el::ByteRange{text.find("namespace"_el), text.find("{"_el).incremented()};
        for (const auto &finding : expected) {
            REQUIRE_EQUAL(finding.range, range);
            REQUIRE_EQUAL(finding.lines, (el::LineRange{el::LineIndex{1}, el::LineCount{1}}));
            REQUIRE_EQUAL(finding.lineNumber(), 2U);
            REQUIRE_EQUAL(finding.endLineNumber(), 2U);
            REQUIRE_EQUAL(finding.snippet, "namespace /* before\0after */ {"_el);
        }
        for (const auto cached : {true, false}) {
            if (!cached) {
                write(".cache/anti_patterns.json"_el, "{}"_el);
            }
            auto scanner = tool::anti_patterns::Scanner{policy, registry, service};
            const auto actual = scanner.scan({}, 4);
            REQUIRE_EQUAL(actual.count(), expected.count());
            if (cached) {
                REQUIRE_EQUAL(scanner.cacheHits(), el::ItemCount{2});
            } else {
                REQUIRE_EQUAL(scanner.cacheMisses(), el::ItemCount{2});
            }
            expected.forEach([&](const tool::anti_patterns::Finding &finding, const el::ItemIndex index) -> void {
                const auto &other = actual.getRef(index);
                REQUIRE_EQUAL(other.rule, finding.rule);
                REQUIRE_EQUAL(other.path, finding.path);
                REQUIRE_EQUAL(other.range, finding.range);
                REQUIRE_EQUAL(other.lines, finding.lines);
                REQUIRE_EQUAL(other.snippet, finding.snippet);
            });
        }
    }

    void testCachePruningPersistsRetainedFindings() {
        using tool::anti_patterns::FindingCache;
        write("src/First.cpp"_el, "namespace {\n}\n"_el);
        write("src/Keep.cpp"_el, "namespace {\n}\n"_el);
        write("src/Last.cpp"_el, "namespace {\n}\n"_el);
        const auto policy = makePolicy();
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto service = files();
        auto scanner = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_EQUAL(scanner.scan({}, 1).count(), el::ItemCount{3});

        const auto first = el::Path{"src/First.cpp"_el};
        const auto keep = el::Path{"src/Keep.cpp"_el};
        const auto last = el::Path{"src/Last.cpp"_el};
        const auto firstSignature = FindingCache::signature(_root / first);
        const auto keepSignature = FindingCache::signature(_root / keep);
        const auto lastSignature = FindingCache::signature(_root / last);
        auto cache = FindingCache{service, registry};
        REQUIRE(cache.get(first, firstSignature) != nullptr);
        REQUIRE(cache.get(keep, keepSignature) != nullptr);
        REQUIRE(cache.get(last, lastSignature) != nullptr);
        cache.retain(el::StringSet{keep.toString()});
        cache.save();

        auto reloaded = FindingCache{service, registry};
        REQUIRE(reloaded.get(first, firstSignature) == nullptr);
        REQUIRE(reloaded.get(last, lastSignature) == nullptr);
        const auto *findings = reloaded.get(keep, keepSignature);
        REQUIRE(findings != nullptr);
        REQUIRE_EQUAL(findings->count(), el::ItemCount{1});
        REQUIRE_EQUAL(findings->first().rule, "anonymous_namespace"_el);
        REQUIRE_EQUAL(findings->first().path, keep);
        reloaded.retain({});
        reloaded.save();
        const auto empty = FindingCache{service, registry};
        REQUIRE(empty.get(keep, keepSignature) == nullptr);
    }

    void testMalformedCacheAndUnsafePaths() {
        write("src/Example.cpp"_el, "namespace {\n}\n"_el);
        const auto policy = makePolicy();
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto service = files();
        write(".cache/anti_patterns.json"_el, "{broken json"_el);
        auto scanner = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_EQUAL(scanner.scan({}, 1).count(), el::ItemCount{1});
        REQUIRE_EQUAL(scanner.cacheMisses(), el::ItemCount{1});
        write(".cache/anti_patterns.json"_el, "{\"version\": 1, \"binary\": \"old\", \"files\": []}"_el);
        auto stale = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_EQUAL(stale.scan({}, 1).count(), el::ItemCount{1});
        REQUIRE_EQUAL(stale.cacheMisses(), el::ItemCount{1});
        auto unsafe = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_THROWS(unsafe.scan(el::StringList{"../outside.cpp"_el}, 1));
        REQUIRE_THROWS(unsafe.scan(el::StringList{(_temporary->path() / "outside.cpp"_el).toString()}, 1));
    }

    void testMalformedConfiguration() {
        write("src/Example.cpp"_el, "namespace {\n}\n"_el);
        for (
            const auto &entry : el::StringList{
                "Path: \"src/*\"\nRules: \"unknown_rule\"\nReason: \"Why.\"\n"_el,
                "Path: \"src/*\"\nRules: \"anonymous_namespace\"\nReason: \"\"\n"_el,
                "Path: \"../outside/*\"\nRules: \"anonymous_namespace\"\nReason: \"Why.\"\n"_el,
                "Path: \"src/[\"\nRules: \"anonymous_namespace\"\nReason: \"Why.\"\n"_el,
                "Path: \"src/*\"\nRules: \"anonymous_namespace\", \"anonymous_namespace\"\nReason: \"Why.\"\n"_el,
            }) {
            REQUIRE_THROWS(makePolicy(el::join("--*[ Anti Patterns . Exception ]*--\n"_el, entry)));
        }
        REQUIRE_THROWS(makePolicy(
            "--*[ Anti Patterns . Exception ]*--\nPath: \"src/*\"\n"
            "Rules: \"anonymous_namespace\"\nReason: \"Why.\"\n"
            "--*[ Anti Patterns . Exception ]*--\nPath: \"src/*\"\n"
            "Rules: \"anonymous_namespace\"\nReason: \"Why.\"\n"_el));
    }

    void testTruncatedTokenLookaheadAndReverseScans() {
        const auto registry = tool::anti_patterns::RuleRegistry{};
        for (
            const auto &text :
            {""_el,
                "class"_el,
                "std::vector<"_el,
                "std::vector<el::"_el,
                "static_cast<void>"_el,
                "\"x\""_el,
                "X::"_el,
                "void f();"_el}) {
            const auto source = tool::anti_patterns::SourceIndex{el::Path{"src/Example.hpp"_el}, text};
            for (const auto &rule : registry.all()) {
                _failureDiagnostic = el::join(rule->identifier(), ": "_el, text);
                auto candidates = el::List<el::ByteRange>{};
                rule->scan(source, candidates);
                for (const auto &range : candidates) {
                    REQUIRE(range.isWithin(text.length()));
                }
            }
        }
    }

    void testPhysicalLineThreshold() {
        const auto registry = tool::anti_patterns::RuleRegistry{};
        auto text = el::StringEditor{};
        for (auto index = el::LineIndex::zero(); index.isWithin(el::LineCount{500}); ++index) {
            text.append("// line\n"_el);
        }
        auto candidates = el::List<el::ByteRange>{};
        registry.find("oversized_file"_el)
            ->scan(tool::anti_patterns::SourceIndex{el::Path{"src/Example.cpp"_el}, el::String{text}}, candidates);
        REQUIRE(candidates.isEmpty());
        const auto start = el::ByteIndex::end(text.length());
        text.append("// extra\n"_el);
        registry.find("oversized_file"_el)
            ->scan(tool::anti_patterns::SourceIndex{el::Path{"src/Example.cpp"_el}, el::String{text}}, candidates);
        REQUIRE_EQUAL(candidates.count(), el::ItemCount{1});
        REQUIRE_EQUAL(candidates.first(), (el::ByteRange{start, el::ByteCount{8}}));
    }

    void testCommandOptionsAndExitStatus() {
        write("src/Example.cpp"_el, "namespace {\n}\n"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[anti_patterns]\nsource_directories: \"src\"\nfile_suffixes: \".cpp\"\n"_el);
        REQUIRE_NOT_EQUAL(
            runTool<tool::anti_patterns::AntiPatternsTool>(
                {}, {}, "anti-patterns"_el, el::StringList{"src/Example.cpp"_el, "--jobs"_el, "1"_el}),
            0);
        REQUIRE(_outputText.contains("1 active"_el));
        REQUIRE_NOT_EQUAL(
            runTool<tool::anti_patterns::AntiPatternsTool>(
                {}, {}, "anti-patterns"_el, el::StringList{"--jobs"_el, "0"_el}),
            0);
        REQUIRE_NOT_EQUAL(
            runTool<tool::anti_patterns::AntiPatternsTool>(
                {}, {}, "anti-patterns"_el, el::StringList{"--limit"_el, "201"_el}),
            0);
        REQUIRE_NOT_EQUAL(
            runTool<tool::anti_patterns::AntiPatternsTool>(
                {}, {}, "anti-patterns"_el, el::StringList{"../outside.cpp"_el}),
            0);
    }

private:
    [[nodiscard]] auto makePolicy(const el::String &exceptions = {}) -> tool::anti_patterns::Policy {
        const auto content = el::join(
            "[Anti Patterns]\nSource Directories: \"src\"\nFile Suffixes: \".cpp\", \".hpp\"\n"_el, exceptions);
        const auto parsed = el::conf::Parser{}.parseTextOrThrow(content);
        auto policy = tool::anti_patterns::Policy{};
        const auto registry = tool::anti_patterns::RuleRegistry{};
        policy.read(parsed->valueOrThrow(cAntiPatternsKey), files(), registry);
        return policy;
    }
};
