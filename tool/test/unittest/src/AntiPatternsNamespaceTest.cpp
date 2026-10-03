// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/StringConverter.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/anti_patterns/RuleRegistry.hpp>
#include <tool/anti_patterns/SourceIndex.hpp>

using namespace el::text::literals;

TESTED_TARGETS(MultipleNamespacesRule SourceIndex)
class AntiPatternsNamespaceTest final : public el::UnitTest {
public: // implement UnitTest
    auto additionalErrorMessages() -> std::string override { return el::StringConverter{_sourceText}.toStdString(); }

public: // tests
    void testDistinctSiblings() {
        for (
            const auto &text : el::StringList{
                "namespace abc {} namespace xyz {}"_el,
                "namespace erbsland::cryptology::unsafe { struct Tag {}; }\n"
                "namespace erbsland::cryptology { class PasswordHasher {}; }"_el,
                "namespace app {} namespace app::impl {}"_el,
                "namespace app::one {} namespace app::two {}"_el,
                "inline namespace first {} namespace second {}"_el,
                "namespace first {} namespace second {} namespace second {} namespace first {}"_el}) {
            WITH_CONTEXT(check(text, el::ItemCount{1}));
        }
        WITH_CONTEXT(check("namespace a {} namespace b {} namespace c {} namespace b {}"_el, el::ItemCount{2}));
    }

    void testSingleNamespaceAndLexicalNoise() {
        for (
            const auto &text : el::StringList{
                "namespace app {} namespace app {}"_el,
                "namespace app::part {} namespace app :: /* separator */ part {}"_el,
                "namespace app::part {} namespace\napp\n:: /* separator */\npart {}"_el,
                "namespace outer { namespace inner {} }"_el,
                "namespace { void helper() {} } namespace app {}"_el,
                "namespace app {} namespace alias = app; using namespace app;"_el,
                "// namespace fake {}\nnamespace app {}\n"
                "const auto text = R\"(namespace another {})\";\n"
                "#define NS namespace macro_space {}\n"_el,
                "namespace app {}\n#ifdef ERBSLAND_OS_WINDOWS\nnamespace platform {}\n#endif\n"_el,
                "#ifdef ERBSLAND_OS_WINDOWS\nnamespace platform {}\n#endif\nnamespace app {}"_el}) {
            WITH_CONTEXT(check(text, el::ItemCount{0}));
        }
    }

    void testStandardSpecializations() {
        for (
            const auto &text : el::StringList{
                "namespace app {} namespace std { template <> struct hash<app::Key> {}; }"_el,
                "namespace std { template <> struct hash<app::Key> {}; } namespace app {}"_el,
                "namespace app {} namespace std { template <typename T> struct formatter<app::Box<T>> "
                ": formatter<int> { void format() {} }; template <> class hash<app::Key> {}; }"_el,
                "namespace app {} namespace std { template <class T> struct hash<app::Box<T>> {}; }"_el,
                "namespace app {} namespace std { /* note */ template /* specialization */ <> "
                "struct formatter<app::Key> {}; /* end */ }"_el,
                "namespace app {} template <> struct std::hash<app::Key> {};"_el,
                "namespace app {} namespace std { template\n<>\nstruct\nhash<app::Key> {}; }"_el,
                "namespace app {}\nnamespace std {\ntemplate <>\nstruct hash<erbsland::network::HttpFieldName> {\n"
                "auto operator()(const erbsland::network::HttpFieldName &value) const noexcept -> std::size_t {\n"
                "return value.toHash();\n}\n};\n}\n"_el}) {
            WITH_CONTEXT(check(text, el::ItemCount{0}));
        }
    }

    void testStandardNamespaceIsNotBlanketExempt() {
        for (
            const auto &body : el::StringList{
                ""_el,
                "struct Other {};"_el,
                "struct hash {};"_el,
                "template <typename T> struct hash {};"_el,
                "template <> struct hash_like<app::Key> {};"_el,
                "void helper(); template <> struct hash<app::Key> {};"_el,
                "template <> struct hash<app::Key> {}; void helper();"_el,
                "template <> struct hash<app::Key> {}; int value; template <> struct formatter<app::Key> {};"_el,
                "template <> struct hash<app::Key> {}; namespace detail {}"_el}) {
            WITH_CONTEXT(check(el::join("namespace app {} namespace std {"_el, body, "}"_el), el::ItemCount{1}));
        }
    }

    void testReportedSpanAndInlineAcceptance() {
        _sourceText = "// ä🙂\nnamespace app {}\n"
                      "// anti-pattern: allow multiple_namespaces -- Isolated test fixture.\n"
                      "namespace app :: /* detail */ other {}\n"_el;
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"src/Example.hpp"_el}, _sourceText};
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto *rule = registry.find("multiple_namespaces"_el);
        REQUIRE(rule != nullptr);
        REQUIRE(rule->severity() == tool::anti_patterns::Severity::High);
        auto candidates = el::List<el::ByteRange>{};
        rule->scan(source, candidates);
        REQUIRE_EQUAL(candidates.count(), el::ItemCount{1});
        REQUIRE_EQUAL(source.slice(candidates.first()), "namespace app :: /* detail */ other {"_el);
        REQUIRE_EQUAL(source.lineIndex(candidates.first().beginIndex()), el::LineIndex{3});
        REQUIRE_EQUAL(source.inlineReason(rule->identifier(), candidates.first()), "Isolated test fixture."_el);
    }

private:
    void check(const el::String &text, const el::ItemCount expected) {
        _sourceText = text;
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"src/Example.cpp"_el}, text};
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto *rule = registry.find("multiple_namespaces"_el);
        REQUIRE(rule != nullptr);
        auto candidates = el::List<el::ByteRange>{};
        rule->scan(source, candidates);
        REQUIRE_EQUAL(candidates.count(), expected);
        for (const auto &range : candidates) {
            REQUIRE(range.isWithin(text.length()));
            REQUIRE(source.slice(range).startsWith("namespace"_el));
        }
    }

private:
    el::String _sourceText; ///< Source retained for failure diagnostics.
};
