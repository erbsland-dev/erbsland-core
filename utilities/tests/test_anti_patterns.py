# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import io
import sys
import tempfile
import time
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.anti_patterns import AntiPatternsApp, report_limit
from lib.anti_patterns import (
    RULES,
    RULES_BY_IDENTIFIER,
    AntiPatternConfig,
    AntiPatternScanner,
    Candidate,
    Finding,
    RuleInfo,
    Severity,
    SourceFile,
    Suppression,
    create_report,
)
from lib.error import UtilityError


def rule_candidates(identifier: str, text: str, path: str = "src/Example.cpp") -> tuple[Candidate, ...]:
    """Run one registered rule on a source sample."""
    source = SourceFile(Path(path), text)
    return tuple(RULES_BY_IDENTIFIER[identifier].scan(source))


def finding(
    identifier: str,
    severity: Severity,
    path: str,
    line: int,
    *,
    suppression: Suppression | None = None,
) -> Finding:
    """Create a report finding for rendering tests."""
    info = RuleInfo(identifier, identifier, severity)
    return Finding(Candidate(info, 0, 1), Path(path), line, line, "example();", suppression)


class SourceFileTest(unittest.TestCase):
    """Tests for the lightweight C++ lexical representation."""

    def test_masks_comments_literals_and_preprocessing_directives(self) -> None:
        source = SourceFile(
            Path("Example.cpp"),
            '#define TEXT "namespace {"\n// namespace {\nauto text = R"tag(namespace {)tag";\nnamespace {\n}\n',
        )

        self.assertNotIn('#define TEXT "namespace {"', source.masked_text)
        self.assertEqual(1, len(source.string_literals))
        self.assertEqual(1, len(rule_candidates("anonymous_namespace", source.text)))

    def test_tracks_nested_class_and_namespace_scopes(self) -> None:
        source = SourceFile(Path("Example.hpp"), "namespace example {\nclass Value {\nclass Nested;\n};\n}\n")
        nested_offset = source.text.index("class Nested")

        self.assertFalse(source.is_namespace_scope(nested_offset))

    def test_finds_headers_with_comparisons_and_trailing_return_types(self) -> None:
        text = (
            "auto previous = value > 1;\n"
            "auto values() noexcept -> std::span<const int> {\n"
            "    if (value >= 1) {\n"
            "    }\n"
            "}\n"
        )
        source = SourceFile(Path("Example.cpp"), text)
        function = next(block for block in source.blocks if "values()" in text[block.header_start : block.start])
        conditional = next(block for block in source.blocks if "if (value" in text[block.header_start : block.start])

        self.assertEqual(text.index(";\n") + 1, function.header_start)
        self.assertEqual(function.start + 1, conditional.header_start)

    def test_tracks_erbsland_os_conditional_blocks(self) -> None:
        text = (
            "callOutside();\n"
            "#ifdef ERBSLAND_OS_WINDOWS\n"
            "callWindows();\n"
            "#if defined(NESTED_FEATURE)\n"
            "callNested();\n"
            "#endif\n"
            "#else\n"
            "callFallback();\n"
            "#endif\n"
            "#if defined(FEATURE)\n"
            "callFeature();\n"
            "#endif\n"
            "#if defined(FEATURE) || \\\n"
            "    defined(ERBSLAND_OS_LINUX)\n"
            "callContinued();\n"
            "#endif\n"
        )
        source = SourceFile(Path("Example.cpp"), text)

        self.assertTrue(source.is_in_os_conditional(text.index("callWindows")))
        self.assertTrue(source.is_in_os_conditional(text.index("callNested")))
        self.assertTrue(source.is_in_os_conditional(text.index("callFallback")))
        self.assertTrue(source.is_in_os_conditional(text.index("callContinued")))
        self.assertFalse(source.is_in_os_conditional(text.index("callOutside")))
        self.assertFalse(source.is_in_os_conditional(text.index("callFeature")))


class AntiPatternRulesTest(unittest.TestCase):
    """Positive and negative tests for every scanner rule."""

    def test_anonymous_namespace(self) -> None:
        self.assertEqual(1, len(rule_candidates("anonymous_namespace", "namespace {\nauto value = 1;\n}\n")))
        self.assertEqual(0, len(rule_candidates("anonymous_namespace", '// namespace {\nauto text = "namespace {";\n')))

    def test_type_in_wrong_unit_detects_namespace_scope_cpp_type_definitions(self) -> None:
        text = (
            "namespace example {\n"
            "class NamespaceType {\n};\n"
            "struct Owner::QualifiedType {\n};\n"
            "class Outer {\n"
            "    struct Nested {\n};\n"
            "};\n"
            "void function() {\n"
            "    class Local {\n};\n"
            "}\n"
            "struct {\nint value;\n} unnamed;\n"
            "}\n"
        )

        candidates = rule_candidates("type_in_wrong_unit", text, "src/example/Types.cpp")

        self.assertEqual(4, len(candidates))
        self.assertEqual(
            {
                text.index("class NamespaceType"),
                text.index("struct Owner::QualifiedType"),
                text.index("class Outer"),
                text.index("struct {"),
            },
            {candidate.start for candidate in candidates},
        )

    def test_type_in_wrong_unit_does_not_report_lexically_nested_types_separately(self) -> None:
        cpp_text = (
            "class Owner {\n"
            "    struct Nested {\n};\n"
            "    void function() {\n"
            "        class MethodLocal {\n};\n"
            "    }\n"
            "};\n"
        )
        header_text = "namespace erbsland::example {\nclass Owner {\nstruct Nested {\n};\n};\n}\n"

        cpp_candidates = rule_candidates("type_in_wrong_unit", cpp_text, "src/example/Owner.cpp")
        header_candidates = rule_candidates("type_in_wrong_unit", header_text, "src/erbsland/example/Owner.hpp")

        self.assertEqual(1, len(cpp_candidates))
        self.assertEqual(cpp_text.index("class Owner"), cpp_candidates[0].start)
        self.assertEqual(0, len(header_candidates))

    def test_type_in_wrong_unit_ignores_cpp_aggregate_variables_and_non_types(self) -> None:
        text = (
            "void function() {\n"
            "    struct stat info{};\n"
            "    struct kevent changes[2]{};\n"
            "    enum class State { Ready };\n"
            "}\n"
            "class Forward;\n"
        )

        self.assertEqual(0, len(rule_candidates("type_in_wrong_unit", text, "src/example/Value.cpp")))

    def test_namespace_in_wrong_unit_checks_domain_and_impl_boundaries(self) -> None:
        valid_samples = (
            ("namespace erbsland::example {\nclass Value {\n};\n}\n", "src/erbsland/example/Value.hpp"),
            (
                "namespace erbsland::example::details {\nclass Value {\n};\n}\n",
                "src/erbsland/example/group/Value.hpp",
            ),
            (
                "namespace erbsland::example::impl::details {\nclass Value {\n};\n}\n",
                "src/erbsland/example/group/impl/Value.hpp",
            ),
            (
                "namespace erbsland {\nnamespace example {\nclass Value {\n};\n}\n}\n",
                "src/erbsland/example/Value.hpp",
            ),
        )
        invalid_samples = (
            ("namespace erbsland::other {\n}\n", "src/erbsland/example/Value.hpp"),
            ("namespace erbsland::example::impl {\n}\n", "src/erbsland/example/Value.hpp"),
            ("namespace erbsland::example {\n}\n", "src/erbsland/example/impl/Value.hpp"),
        )

        for text, path in valid_samples:
            with self.subTest(path=path, text=text):
                self.assertEqual(0, len(rule_candidates("namespace_in_wrong_unit", text, path)))
        for text, path in invalid_samples:
            with self.subTest(path=path, text=text):
                self.assertEqual(1, len(rule_candidates("namespace_in_wrong_unit", text, path)))

    def test_namespace_in_wrong_unit_reports_the_namespace_declaration(self) -> None:
        text = "namespace erbsland::cterm {\n}\n"

        candidates = rule_candidates("namespace_in_wrong_unit", text, "src/erbsland/cterm/impl/TypeTraits.hpp")

        self.assertEqual(1, len(candidates))
        self.assertEqual(text.index("namespace erbsland::cterm"), candidates[0].start)

    def test_type_in_wrong_unit_checks_qualified_namespace_on_type(self) -> None:
        text = "namespace erbsland::example {\nclass impl::Value {\n};\n}\n"

        candidates = rule_candidates("type_in_wrong_unit", text, "src/erbsland/example/Value.hpp")
        cpp_candidates = rule_candidates("type_in_wrong_unit", text, "src/erbsland/example/Value.cpp")

        self.assertEqual(1, len(candidates))
        self.assertEqual(text.index("class impl::Value"), candidates[0].start)
        self.assertEqual(1, len(cpp_candidates))
        self.assertEqual(text.index("class impl::Value"), cpp_candidates[0].start)

    def test_type_in_wrong_unit_allows_supported_standard_library_specializations(self) -> None:
        text = (
            "namespace erbsland::example {\nclass Value {\n};\n}\n"
            "template <>\nstruct std::hash<erbsland::example::Value> {\n};\n"
        )

        self.assertEqual(
            0,
            len(rule_candidates("type_in_wrong_unit", text, "src/erbsland/example/Value.hpp")),
        )

    def test_implementation_in_wrong_unit_accepts_primary_and_split_units(self) -> None:
        primary = (
            "namespace erbsland::example {\n"
            "Foo::Foo() = default;\n"
            "Foo::~Foo() = default;\n"
            "auto Foo::value() const -> int {\nreturn 1;\n}\n"
            "auto Foo::operator==(const Foo &) const -> bool {\nreturn true;\n}\n"
            "}\n"
        )
        split = "namespace erbsland::example {\nauto Foo::Nested::parse() -> int {\nreturn 1;\n}\n}\n"

        self.assertEqual(0, len(rule_candidates("implementation_in_wrong_unit", primary, "src/Foo.cpp")))
        self.assertEqual(0, len(rule_candidates("implementation_in_wrong_unit", split, "src/Foo_parsing.cpp")))

    def test_implementation_in_wrong_unit_detects_obvious_member_definitions(self) -> None:
        text = (
            "namespace erbsland::example {\n"
            "Foo::Foo() = default;\n"
            "Foo::~Foo() = default;\n"
            "auto Foo::value(\n"
            "    int input) const -> int {\nreturn input;\n}\n"
            "auto impl::Foo::operator==(const Foo &) const -> bool {\nreturn true;\n}\n"
            "}\n"
        )

        candidates = rule_candidates("implementation_in_wrong_unit", text, "src/Bar.cpp")

        self.assertEqual(4, len(candidates))
        self.assertEqual(
            {
                text.index("Foo::Foo"),
                text.index("Foo::~Foo"),
                text.index("Foo::value"),
                text.index("impl::Foo::operator=="),
            },
            {candidate.start for candidate in candidates},
        )

    def test_implementation_in_wrong_unit_ignores_calls_and_free_functions(self) -> None:
        text = (
            "namespace erbsland::example {\n"
            "auto value = Foo::create();\n"
            "void freeFunction() {\n"
            "    Foo::run();\n"
            "    auto nested = impl::Foo::create();\n"
            "}\n"
            "}\n"
        )

        self.assertEqual(0, len(rule_candidates("implementation_in_wrong_unit", text, "src/Bar.cpp")))

    def test_implementation_in_wrong_unit_scales_over_masked_preprocessor_lines(self) -> None:
        text = "#define GENERATED() \\\n" + (" " * 120 + "\\\n") * 64 + "void end();\n"
        source = SourceFile(Path("src/Generated.cpp"), text)
        rule = RULES_BY_IDENTIFIER["implementation_in_wrong_unit"]

        start = time.perf_counter()
        candidates = tuple(rule.scan(source))
        elapsed = time.perf_counter() - start

        self.assertEqual((), candidates)
        self.assertLess(elapsed, 0.5)

    def test_oversized_nested_type_uses_a_ten_code_line_limit(self) -> None:
        ten_lines = (
            "class Owner {\n"
            "    class Nested {\n"
            "    public:\n"
            "        int a;\n"
            "        int b;\n"
            "        // Excluded comment.\n"
            "#define EXCLUDED_DIRECTIVE 1\n"
            "\n"
            "        int c;\n"
            "        int d;\n"
            "        int e;\n"
            "        int f;\n"
            "        int g;\n"
            "    };\n"
            "};\n"
        )
        eleven_lines = ten_lines.replace("        int g;\n", "        int g;\n        int h;\n")

        self.assertEqual(0, len(rule_candidates("oversized_nested_type", ten_lines, "src/Owner.hpp")))
        candidates = rule_candidates("oversized_nested_type", eleven_lines, "src/Owner.hpp")
        self.assertEqual(1, len(candidates))
        self.assertEqual(eleven_lines.index("class Nested"), candidates[0].start)

    def test_oversized_nested_type_counts_multiline_declarations_and_deeper_types(self) -> None:
        multiline = (
            "class Owner {\n"
            "    struct Nested\n"
            "        final\n"
            "        : public Base\n"
            "    {\n"
            "        int a;\n"
            "        int b;\n"
            "        int c;\n"
            "        int d;\n"
            "        int e;\n"
            "        int f;\n"
            "    };\n"
            "};\n"
        )
        deeper = (
            "class Outer {\n"
            "    class Middle {\n"
            "        struct Inner {\n"
            "            int a;\n"
            "            int b;\n"
            "            int c;\n"
            "            int d;\n"
            "            int e;\n"
            "            int f;\n"
            "            int g;\n"
            "            int h;\n"
            "            int i;\n"
            "        };\n"
            "    };\n"
            "};\n"
        )

        self.assertEqual(1, len(rule_candidates("oversized_nested_type", multiline, "src/Owner.hpp")))
        candidates = rule_candidates("oversized_nested_type", deeper, "src/Outer.hpp")
        self.assertEqual(
            {deeper.index("class Middle"), deeper.index("struct Inner")},
            {candidate.start for candidate in candidates},
        )

    def test_oversized_nested_type_excludes_function_local_types(self) -> None:
        text = (
            "class Owner {\n"
            "    void run() {\n"
            "        class Local {\n"
            "            int a;\n"
            "            int b;\n"
            "            int c;\n"
            "            int d;\n"
            "            int e;\n"
            "            int f;\n"
            "            int g;\n"
            "            int h;\n"
            "            int i;\n"
            "        };\n"
            "    }\n"
            "};\n"
        )

        self.assertEqual(0, len(rule_candidates("oversized_nested_type", text, "src/Owner.hpp")))

    def test_oversized_nested_type_detects_out_of_line_definitions(self) -> None:
        members = "".join(f"    int value{index};\n" for index in range(9))
        regular = f"class Owner::Nested {{\n{members}}};\n"
        templated = f"class Owner<T>::Nested {{\n{members}}};\n"

        self.assertEqual(1, len(rule_candidates("oversized_nested_type", regular)))
        self.assertEqual(1, len(rule_candidates("oversized_nested_type", templated)))

    def test_oversized_nested_type_excludes_namespace_qualified_types(self) -> None:
        members = "".join(f"    int value{index};\n" for index in range(9))
        text = (
            f"class example::Value {{\n{members}}};\n"
            f"struct WidthTraits<StringWidth::U8> {{\n{members}}};\n"
            f"struct std::hash<Value> {{\n{members}}};\n"
            f"struct std::formatter<Value> {{\n{members}}};\n"
        )

        self.assertEqual(0, len(rule_candidates("oversized_nested_type", text)))

    def test_forward_declaration(self) -> None:
        self.assertEqual(1, len(rule_candidates("forward_declaration", "namespace example {\nclass Value;\n}\n")))
        self.assertEqual(
            0,
            len(rule_candidates("forward_declaration", "class Owner {\nclass Nested;\n};\n", "src/Owner.hpp")),
        )
        self.assertEqual(
            0,
            len(rule_candidates("forward_declaration", "class Value;\n", "src/Value_fwd.hpp")),
        )

    def test_static_global_object(self) -> None:
        text = "Widget globalWidget{1};\nconstexpr Widget constantWidget{1};\nstatic int count = 1;\n"
        candidates = rule_candidates("static_global_object", text)

        self.assertEqual(1, len(candidates))
        self.assertEqual(text.index("Widget globalWidget"), candidates[0].start)

    def test_static_global_object_excludes_class_members_and_function_statics(self) -> None:
        text = (
            "class Owner {\nWidget member{1};\n};\n"
            "auto function(\nWidget defaultValue = Widget{}) -> void {\nstatic Widget local{1};\n}\n"
            "template <Widget defaultValue = Widget{}>\nclass Template;\n"
        )

        self.assertEqual(0, len(rule_candidates("static_global_object", text)))

    def test_static_only_class(self) -> None:
        static_only = "class Tools {\npublic:\nstatic void run();\nTools() = delete;\n};\n"
        instance = "class Value {\npublic:\nvoid run();\n};\n"

        self.assertEqual(1, len(rule_candidates("static_only_class", static_only, "src/Tools.hpp")))
        self.assertEqual(0, len(rule_candidates("static_only_class", instance, "src/Value.hpp")))

    def test_missing_api_documentation(self) -> None:
        missing = "auto first() -> void;\nauto second() -> void;\n"
        documented = "auto first() -> void;\n/// Run the second operation.\nauto second() -> void;\n"
        standard_formatter = (
            "template <typename T>\n"
            "struct std::formatter<Example<T>> {\n"
            "using Base = std::formatter<int>;\n\n"
            "auto format(const Example<T> &, std::format_context &) const -> std::format_context::iterator;\n"
            "};\n"
        )
        standard_hash = (
            "template <>\n"
            "struct std::hash<Example> {\n"
            "    auto operator()(const Example &) const noexcept -> std::size_t;\n"
            "};\n"
        )
        unqualified_standard_hash = (
            "namespace std {\n"
            "template <>\n"
            "struct hash<Example> {\n"
            "    auto operator()(const Example &) const noexcept -> std::size_t;\n"
            "};\n"
            "}\n"
        )
        nonstandard_hash = (
            "namespace example {\n"
            "template <typename T>\n"
            "struct hash<Example<T>> {\n"
            "    auto operator()(const T &) const noexcept -> std::size_t;\n"
            "};\n"
            "}\n"
        )
        constrained_template = (
            "/// Convert the value.\n"
            "template <typename T>\n"
            "    requires Example<T> &&\n"
            "    SupportsConversion<T>\n"
            "auto convert() -> T;\n"
        )
        multiline_template = (
            "/// Store a pair of values.\n"
            "template <\n"
            "    typename First,\n"
            "    typename Second>\n"
            "class Pair {\n"
            "};\n"
            "/// Convert a pair of values.\n"
            "template <\n"
            "    typename First,\n"
            "    typename Second>\n"
            "auto convertPair(First first, Second second) -> Pair<First, Second>;\n"
        )
        defaults_group = (
            "/// Store a defaultable value.\n"
            "class Value {\n"
            "    // defaults\n"
            "    auto operator=(const Value &) -> Value & = default; // Preserve the value.\n"
            "    auto operator=(Value &&) noexcept -> Value & = delete;\n"
            "};\n"
        )
        undocumented_default = (
            "/// Run the first operation.\n"
            "auto first() -> void;\n"
            "auto operator=(const Value &) -> Value & = default;\n"
            "auto operator=(Value &&) = delete;\n"
        )
        defaulted_comparison = "auto operator==(const Value &) -> bool = default;\n"
        private_tag = "struct PrivateTag {};\nstruct Undocumented {};\n"
        private_class_tag = "class PrivateTag {};\nstruct Undocumented {};\n"
        overrides = (
            "/// Implement the example interface.\n"
            "class Derived {\n"
            "public:\n"
            "    [[nodiscard]] auto documented() const -> int override;\n"
            "    void undocumented() override;\n"
            "    [[nodiscard]] auto ownOperation() const -> int;\n"
            "};\n"
        )

        self.assertEqual(2, len(rule_candidates("missing_api_documentation", missing, "src/Functions.hpp")))
        self.assertEqual(1, len(rule_candidates("missing_api_documentation", documented, "src/Functions.hpp")))
        self.assertEqual(
            0,
            len(rule_candidates("missing_api_documentation", standard_formatter, "src/StdFormat.hpp")),
        )
        self.assertEqual(0, len(rule_candidates("missing_api_documentation", standard_hash, "src/Hash.hpp")))
        self.assertEqual(
            0,
            len(rule_candidates("missing_api_documentation", unqualified_standard_hash, "src/Hash.hpp")),
        )
        self.assertEqual(2, len(rule_candidates("missing_api_documentation", nonstandard_hash, "src/Hash.hpp")))
        self.assertEqual(0, len(rule_candidates("missing_api_documentation", constrained_template, "src/Value.hpp")))
        self.assertEqual(0, len(rule_candidates("missing_api_documentation", multiline_template, "src/Pair.hpp")))
        self.assertEqual(0, len(rule_candidates("missing_api_documentation", defaults_group, "src/Value.hpp")))
        self.assertEqual(0, len(rule_candidates("missing_api_documentation", undocumented_default, "src/Value.hpp")))
        self.assertEqual(
            0,
            len(rule_candidates("missing_api_documentation", defaulted_comparison, "src/Value.hpp")),
        )
        self.assertEqual(1, len(rule_candidates("missing_api_documentation", private_tag, "src/Value.hpp")))
        self.assertEqual(1, len(rule_candidates("missing_api_documentation", private_class_tag, "src/Value.hpp")))
        candidates = rule_candidates("missing_api_documentation", overrides, "src/Derived.hpp")
        self.assertEqual(1, len(candidates))
        self.assertEqual(overrides.index("[[nodiscard]] auto ownOperation"), candidates[0].start)
        self.assertEqual(0, len(rule_candidates("missing_api_documentation", missing, "src/Functions.cpp")))

    def test_missing_api_documentation_detects_conf_version_constructors(self) -> None:
        undocumented = (
            "/// Restrict a rule to specific versions.\n"
            "class ConfVersion {\n"
            "public:\n"
            "    explicit ConfVersion(std::vector<Integer> versions, const bool isNegated = false) :\n"
            "        _versions{std::move(versions)}, _isNegated{isNegated} {}\n"
            "    explicit ConfVersion(const std::initializer_list<Integer> versions, const bool isNegated = false) :\n"
            "        _versions{versions}, _isNegated{isNegated} {}\n"
            "    explicit ConfVersion(const Integer version, const bool isNegated = false) :\n"
            "        _versions{version}, _isNegated{isNegated} {}\n"
            "};\n"
        )
        documented = (
            "/// Restrict a rule to specific versions.\n"
            "class ConfVersion {\n"
            "public:\n"
            "    /// Create a restriction from a list of versions.\n"
            "    explicit ConfVersion(std::vector<Integer> versions, const bool isNegated = false);\n"
            "    /// @overload\n"
            "    explicit ConfVersion(const std::initializer_list<Integer> versions, const bool isNegated = false);\n"
            "    /// @overload\n"
            "    explicit ConfVersion(const Integer version, const bool isNegated = false);\n"
            "};\n"
        )

        candidates = rule_candidates("missing_api_documentation", undocumented, "src/ConfVersion.hpp")
        first = undocumented.index("explicit ConfVersion")
        second = undocumented.index("explicit ConfVersion", first + 1)
        third = undocumented.index("explicit ConfVersion", second + 1)

        self.assertEqual(3, len(candidates))
        self.assertEqual([first, second, third], [candidate.start for candidate in candidates])
        self.assertEqual(
            0,
            len(rule_candidates("missing_api_documentation", documented, "src/ConfVersion.hpp")),
        )

    def test_missing_api_documentation_detects_special_member_forms(self) -> None:
        text = (
            "/// Store an example value.\n"
            "class Value {\n"
            "public:\n"
            "    Value();\n"
            "    explicit Value(\n"
            "        int first,\n"
            "        int second);\n"
            "    Value(const Value &other);\n"
            "    Value(Value &&other) noexcept;\n"
            "    template <typename T>\n"
            "        requires Example<T>\n"
            "    explicit Value(T value);\n"
            "    ~Value();\n"
            "private:\n"
            "    [[nodiscard]] explicit Value(PrivateTag);\n"
            "};\n"
        )

        candidates = rule_candidates("missing_api_documentation", text, "src/Value.hpp")

        self.assertEqual(7, len(candidates))
        self.assertEqual(6, sum("~Value(" not in text[candidate.start : candidate.end] for candidate in candidates))
        self.assertEqual(1, sum("~Value(" in text[candidate.start : candidate.end] for candidate in candidates))

    def test_missing_api_documentation_leaves_defaulted_members_to_group_rule(self) -> None:
        grouped = (
            "/// Store an example value.\n"
            "class Value {\n"
            "public:\n"
            "    // defaults/deletions\n"
            "    Value() = default;\n"
            "    ~Value() = default;\n"
            "    Value(const Value &) = delete;\n"
            "    Value(Value &&) = default;\n"
            "    auto operator=(const Value &) -> Value & = delete;\n"
            "};\n"
        )
        ungrouped = grouped.replace("    // defaults/deletions\n", "")

        self.assertEqual(0, len(rule_candidates("missing_api_documentation", grouped, "src/Value.hpp")))
        self.assertEqual(0, len(rule_candidates("missing_default_group_comment", grouped, "src/Value.hpp")))
        self.assertEqual(0, len(rule_candidates("missing_api_documentation", ungrouped, "src/Value.hpp")))
        self.assertEqual(5, len(rule_candidates("missing_default_group_comment", ungrouped, "src/Value.hpp")))

    def test_missing_api_documentation_trusts_defaults_group_for_out_of_line_defaults(self) -> None:
        grouped = (
            "/// Store an example value.\n"
            "class Value {\n"
            "public:\n"
            "    // defaults\n"
            "    Value();\n"
            "    ~Value();\n"
            "    Value(const Value &);\n"
            "    Value(Value &&) noexcept;\n"
            "    auto operator=(const Value &) -> Value &;\n"
            "    auto operator=(Value &&) noexcept -> Value &;\n"
            "};\n"
        )
        ungrouped = grouped.replace("    // defaults\n", "")

        self.assertEqual(0, len(rule_candidates("missing_api_documentation", grouped, "src/Value.hpp")))
        self.assertEqual(6, len(rule_candidates("missing_api_documentation", ungrouped, "src/Value.hpp")))

    def test_missing_api_documentation_defaults_group_only_covers_default_special_members(self) -> None:
        text = (
            "/// Store an example value.\n"
            "class Value {\n"
            "public:\n"
            "    // defaults\n"
            "    Value();\n"
            "    /// Reset the value.\n"
            "    void reset();\n"
            "    Value(int value);\n"
            "};\n"
        )

        candidates = rule_candidates("missing_api_documentation", text, "src/Value.hpp")

        self.assertEqual(1, len(candidates))
        self.assertEqual(text.index("Value(int value)"), candidates[0].start)

    def test_missing_api_documentation_rejects_constructor_lookalikes(self) -> None:
        text = (
            "/// Store an example value.\n"
            "class Value {\n"
            "public:\n"
            "    /// Create a value through a delegating constructor.\n"
            "    Value(int value) :\n"
            "        Value(value, 0) {}\n"
            "    /// Store a shared example.\n"
            "    inline static auto cExample =\n"
            "        Value(1);\n"
            "    /// Store a nested shared example.\n"
            "    inline static auto cNestedExample = createShared(\n"
            "        Value(2));\n"
            "    /// Store a conditional shared example.\n"
            "    inline static auto cConditionalExample = condition ?\n"
            "        Value(3) :\n"
            "        Value(4);\n"
            "    /// Create a temporary value.\n"
            "    static auto create() -> Value { return Value(1); }\n"
            "};\n"
            "Value(int) -> Value;\n"
            "/// Exercise local construction.\n"
            "void exercise() {\n"
            "    Value(1);\n"
            "}\n"
        )
        overriding_destructor = (
            "/// Implement a base interface.\n" "class Derived {\n" "public:\n" "    ~Derived() override;\n" "};\n"
        )

        self.assertEqual(0, len(rule_candidates("missing_api_documentation", text, "src/Value.hpp")))
        self.assertEqual(
            0,
            len(rule_candidates("missing_api_documentation", overriding_destructor, "src/Derived.hpp")),
        )
        self.assertEqual(
            0,
            len(rule_candidates("missing_api_documentation", "class Value { Value(); };\n", "src/Value_fwd.hpp")),
        )
        self.assertEqual(
            0,
            len(rule_candidates("missing_api_documentation", "class Value { Value(); };\n", "src/Value.cpp")),
        )

    def test_missing_default_group_comment(self) -> None:
        grouped = (
            "class Value {\n"
            "    // defaults/deletions\n"
            "    Value() = default;\n"
            "    ~Value() override = default;\n"
            "    Value(const Value &) = delete;\n"
            "    auto operator=(const Value &) -> Value & = delete;\n"
            "    auto operator==(const Value &) const -> bool = default;\n"
            "};\n"
        )
        ungrouped = (
            "class Value {\n"
            "    Value() = default;\n"
            "    ~Value() override = default;\n"
            "    Value(const Value &) = delete;\n"
            "    auto operator=(const Value &) -> Value & = delete;\n"
            "    auto operator==(const Value &) const -> bool = default;\n"
            "};\n"
        )
        documented_default_constructor = (
            "class Value {\n"
            "    /// Create an empty value that is ready for lazy initialization.\n"
            "    Value() = default;\n"
            "};\n"
        )
        documented_copy_constructor = (
            "class Value {\n" "    /// Copy another value.\n" "    Value(const Value &) = default;\n" "};\n"
        )
        grouped_templated_deletion = (
            "template <typename T>\n"
            "class Value {\n"
            "    // defaults/deletions\n"
            "    Value() = default;\n"
            "    template <\n"
            "        typename Other,\n"
            "        typename Enable = void>\n"
            "        requires Example<Other>\n"
            "    explicit Value(Other value) = delete;\n"
            "    Value(const Value &) = default;\n"
            "};\n"
        )

        self.assertEqual(0, len(rule_candidates("missing_default_group_comment", grouped, "src/Value.hpp")))
        self.assertEqual(
            0,
            len(rule_candidates("missing_default_group_comment", documented_default_constructor, "src/Value.hpp")),
        )
        self.assertEqual(
            1,
            len(rule_candidates("missing_default_group_comment", documented_copy_constructor, "src/Value.hpp")),
        )
        self.assertEqual(
            0,
            len(rule_candidates("missing_default_group_comment", grouped_templated_deletion, "src/Value.hpp")),
        )
        candidates = rule_candidates("missing_default_group_comment", ungrouped, "src/Value.hpp")
        self.assertEqual(4, len(candidates))
        self.assertEqual(ungrouped.index("Value() = default"), candidates[0].start)
        self.assertEqual(ungrouped.index("~Value() override = default"), candidates[1].start)
        self.assertEqual(ungrouped.index("Value(const Value &) = delete"), candidates[2].start)
        self.assertEqual(ungrouped.index("auto operator=(const Value &)"), candidates[3].start)

    def test_missing_api_documentation_ignores_local_state_and_handles_declaration_preambles(self) -> None:
        text = (
            "namespace example {\n"
            "/// Create a templated value.\n"
            "template <typename T>\n"
            "[[nodiscard]] auto documentedTemplate(T value) -> T;\n"
            "/// Create a constrained templated value.\n"
            "template <typename T>\n"
            "requires(sizeof(T) > 0)\n"
            "[[nodiscard]] auto documentedConstrainedTemplate(T value) -> T;\n"
            "template <typename T>\n"
            "[[nodiscard]] auto undocumentedTemplate(T value) -> T;\n"
            "/// Store example operations.\n"
            "class Example {\n"
            "public:\n"
            "    friend class Friend;\n"
            "    struct Forward;\n"
            "    static constexpr auto cValue = 1;\n"
            "    /// Run the documented operation.\n"
            "    void documentedMember();\n"
            "    void undocumentedMember();\n"
            "    /// Exercise local state without declaring an API.\n"
            "    void localState() {\n"
            "        auto first = 1;\n"
            "        auto second = first + 1;\n"
            "    }\n"
            "};\n"
            "}\n"
        )

        candidates = rule_candidates("missing_api_documentation", text, "src/Example.hpp")

        self.assertEqual(2, len(candidates))
        self.assertEqual(text.index("[[nodiscard]] auto undocumentedTemplate"), candidates[0].start)
        self.assertEqual(text.index("void undocumentedMember"), candidates[1].start)

    def test_multiple_types_in_header(self) -> None:
        text = "class Primary {\n};\nclass Additional {\n};\n"

        self.assertEqual(1, len(rule_candidates("multiple_types_in_header", text, "src/Primary.hpp")))

    def test_multiple_types_allows_primary_template_specializations(self) -> None:
        text = (
            "namespace example {\n"
            "template <typename T>\n"
            "class Primary {\n};\n"
            "template <typename T>\n"
            "    requires(sizeof(T) > 1)\n"
            "class Primary<T *> {\n};\n"
            "template <>\n"
            "class Primary<void> {\n};\n"
            "}\n"
        )

        self.assertEqual(0, len(rule_candidates("multiple_types_in_header", text, "src/Primary.hpp")))

    def test_multiple_types_reports_type_unrelated_to_primary_template(self) -> None:
        text = (
            "template <typename T>\n"
            "class Primary {\n};\n"
            "template <>\n"
            "class Primary<void> {\n};\n"
            "class Additional {\n};\n"
        )

        candidates = rule_candidates("multiple_types_in_header", text, "src/Primary.hpp")

        self.assertEqual(1, len(candidates))
        self.assertEqual(text.index("class Additional"), candidates[0].start)

    def test_multiple_types_rejects_specialization_in_different_namespace(self) -> None:
        text = (
            "namespace first {\n"
            "template <typename T>\n"
            "class Primary {\n};\n"
            "}\n"
            "namespace second {\n"
            "template <>\n"
            "class Primary<void> {\n};\n"
            "}\n"
        )

        self.assertEqual(1, len(rule_candidates("multiple_types_in_header", text, "src/Primary.hpp")))

    def test_multiple_types_matches_equivalent_namespace_syntax(self) -> None:
        text = (
            "namespace outer::inner {\n"
            "template <typename T>\n"
            "class Primary {\n};\n"
            "}\n"
            "namespace outer {\n"
            "namespace inner {\n"
            "template <>\n"
            "class Primary<void> {\n};\n"
            "}\n"
            "}\n"
        )

        self.assertEqual(0, len(rule_candidates("multiple_types_in_header", text, "src/Primary.hpp")))

    def test_multiple_types_rejects_same_named_regular_types(self) -> None:
        text = "class Primary {\n};\nclass Primary {\n};\n"

        self.assertEqual(1, len(rule_candidates("multiple_types_in_header", text, "src/Primary.hpp")))

    def test_multiple_types_allows_standard_library_specializations(self) -> None:
        text = (
            "class Primary {\n};\n"
            "template <>\n"
            "struct std::hash<Primary> {\n"
            "auto operator()(const Primary &) const -> std::size_t;\n"
            "};\n"
        )

        self.assertEqual(0, len(rule_candidates("multiple_types_in_header", text, "src/Primary.hpp")))

        unqualified_hash = (
            "class Primary {\n};\n"
            "namespace std {\n"
            "template <typename T>\n"
            "struct hash<Primary<T>> {\n"
            "auto operator()(const Primary<T> &) const -> std::size_t;\n"
            "};\n"
            "}\n"
        )

        self.assertEqual(0, len(rule_candidates("multiple_types_in_header", unqualified_hash, "src/Primary.hpp")))

        nonstandard_hash = (
            "class Primary {\n};\n"
            "namespace example {\n"
            "template <typename T>\n"
            "struct hash<Primary<T>> {\n"
            "auto operator()(const T &) const -> std::size_t;\n"
            "};\n"
            "}\n"
        )

        self.assertEqual(1, len(rule_candidates("multiple_types_in_header", nonstandard_hash, "src/Primary.hpp")))

    def test_multiple_types_allows_small_internal_helper(self) -> None:
        text = "class Primary {\n};\nstruct Helper {\nint value;\n};\n"

        self.assertEqual(
            0,
            len(rule_candidates("multiple_types_in_header", text, "src/example/impl/Primary.hpp")),
        )

    def test_regular_string_literal(self) -> None:
        text = (
            'call("copy");\n'
            'call("borrowed"_el);\n'
            'call(el::StringLiteral{"direct"});\n'
            'call(el::StringLiteral("parenthesized"));\n'
            'call(el::U8StringLiteral<char>{"typed"});\n'
            'call(el::String{"intentional"});\n'
            'throw std::runtime_error{"intentional"};\n'
            '#define VALUE "macro"\n'
        )
        candidates = rule_candidates("regular_string_literal", text)

        self.assertEqual(1, len(candidates))
        self.assertEqual(text.index('"copy"'), candidates[0].start)

    def test_regular_string_literal_allows_assertion_messages(self) -> None:
        text = (
            'static_assert(sizeof(int) > 1, "unsupported integer");\n'
            'static_assert(check("nested compile-time value"), "nested message");\n'
            'assert(value && "missing value");\n'
            'assert(check("nested runtime value"));\n'
        )

        self.assertEqual(0, len(rule_candidates("regular_string_literal", text)))

    def test_regular_string_literal_allows_deprecation_messages(self) -> None:
        text = (
            '[[deprecated("use replacement()")]] void oldFunction();\n'
            '[[nodiscard, deprecated("use newValue()")]] auto oldValue() -> int;\n'
        )

        self.assertEqual(0, len(rule_candidates("regular_string_literal", text, "src/Example.hpp")))

    def test_regular_string_literal_rejects_similarly_named_calls(self) -> None:
        text = (
            'custom_assert("ordinary argument");\n'
            'static_assertion("ordinary argument");\n'
            'deprecated("ordinary argument");\n'
            'assert(condition;\ncall("after malformed assertion");\n'
        )

        self.assertEqual(4, len(rule_candidates("regular_string_literal", text)))

    def test_static_cast_void(self) -> None:
        text = "static_cast<void>(operation());\n// static_cast<void>(ignored());\n"

        self.assertEqual(1, len(rule_candidates("static_cast_void", text)))

    def test_nested_namespace(self) -> None:
        nested = "namespace erbsland {\n" "namespace example {\n" "}\n" "inline namespace current {\n" "}\n" "}\n"

        self.assertEqual(2, len(rule_candidates("nested_namespace", nested)))
        self.assertEqual(0, len(rule_candidates("nested_namespace", "namespace erbsland::example::sub {\n}\n")))

    def test_nested_namespace_does_not_duplicate_anonymous_namespace_findings(self) -> None:
        text = "namespace erbsland {\nnamespace {\nauto value = 1;\n}\n}\n"

        self.assertEqual(1, len(rule_candidates("anonymous_namespace", text)))
        self.assertEqual(0, len(rule_candidates("nested_namespace", text)))

    def test_nested_namespace_detects_namespace_end_comments(self) -> None:
        text = (
            "namespace first {\n"
            "} // any trailing text\n"
            "namespace second {\n"
            "} /* namespace second */\n"
            "class Value {\n"
            "}; // class Value\n"
            "void function() {\n"
            "} // function\n"
        )
        candidates = rule_candidates("nested_namespace", text)

        self.assertEqual(2, len(candidates))
        self.assertEqual(
            {text.index("// any trailing text"), text.index("/* namespace second */")},
            {candidate.start for candidate in candidates},
        )


class SuppressionTest(unittest.TestCase):
    """Tests for adjacent inline suppression markers."""

    def test_accepts_preceding_and_trailing_markers_with_reasons(self) -> None:
        preceding = SourceFile(
            Path("Example.cpp"),
            "// anti-pattern: allow static_cast_void -- Validated before this call.\n"
            "static_cast<void>(operation());\n",
        )
        trailing = SourceFile(
            Path("Example.cpp"),
            "static_cast<void>(operation()); // anti-pattern: allow static_cast_void -- Validated before this call.\n",
        )
        rule = RULES_BY_IDENTIFIER["static_cast_void"]

        for source in (preceding, trailing):
            candidate = next(iter(rule.scan(source)))
            self.assertEqual(
                "Validated before this call.",
                source.inline_suppression(rule.info.identifier, candidate.start, candidate.end),
            )

    def test_rejects_missing_reason_wrong_rule_and_nonadjacent_marker(self) -> None:
        samples = (
            "// anti-pattern: allow static_cast_void --\nstatic_cast<void>(operation());\n",
            "// anti-pattern: allow anonymous_namespace -- A reason.\nstatic_cast<void>(operation());\n",
            "// anti-pattern: allow static_cast_void -- A reason.\n\nstatic_cast<void>(operation());\n",
        )
        rule = RULES_BY_IDENTIFIER["static_cast_void"]

        for text in samples:
            source = SourceFile(Path("Example.cpp"), text)
            candidate = next(iter(rule.scan(source)))
            self.assertIsNone(source.inline_suppression(rule.info.identifier, candidate.start, candidate.end))

    def test_accepts_new_rule_suppressions_at_definition_and_namespace_end(self) -> None:
        members = "".join(f"    int value{index};\n" for index in range(9))
        nested_type = SourceFile(
            Path("Example.cpp"),
            f"class Owner::Nested {{\n{members}}}; "
            "// anti-pattern: allow oversized_nested_type -- Language-required nested type.\n",
        )
        namespace_end = SourceFile(
            Path("Example.cpp"),
            "namespace example {\n"
            "// anti-pattern: allow nested_namespace -- Imported source convention.\n"
            "} // namespace example\n",
        )

        for identifier, source, reason in (
            ("oversized_nested_type", nested_type, "Language-required nested type."),
            ("nested_namespace", namespace_end, "Imported source convention."),
        ):
            rule = RULES_BY_IDENTIFIER[identifier]
            candidate = next(iter(rule.scan(source)))
            self.assertEqual(reason, source.inline_suppression(identifier, candidate.start, candidate.end))


class AntiPatternConfigTest(unittest.TestCase):
    """Tests for ELCL scanner configuration and central suppressions."""

    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_directory = Path(self.temporary_directory.name)
        (self.project_directory / "src/tests").mkdir(parents=True)
        (self.project_directory / "src/Example.cpp").write_text("namespace {\n}\n", encoding="utf-8")
        (self.project_directory / "src/tests/Test.cpp").write_text(
            "static_cast<void>(operation());\n", encoding="utf-8"
        )

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def write_config(self, extra: str = "") -> Path:
        """Write a complete scanner configuration for one test."""
        path = self.project_directory / "anti_patterns.elcl"
        path.write_text(
            '[Main]\nSource Directories:\n    * "src"\nFile Suffixes:\n    * ".cpp"\n' + extra,
            encoding="utf-8",
        )
        return path

    def test_reads_literal_directory_and_file_suppressions(self) -> None:
        config_path = self.write_config(
            "*[Rule.static_cast_void]*\n"
            'Excluded Path: "src/tests"\nReason: "Unit-test exception."\n'
            "*[Rule.anonymous_namespace]*\n"
            'Excluded Path: "src/Example.cpp"\nReason: "Legacy exception."\n'
            "*[Rule.oversized_nested_type]*\n"
            'Excluded Path: "src/tests"\nReason: "Language-required nested types."\n'
            "*[Rule.nested_namespace]*\n"
            'Excluded Path: "src/Example.cpp"\nReason: "Imported namespace style."\n'
        )

        config = AntiPatternConfig.read(self.project_directory, config_path)

        self.assertIsNotNone(config.suppression_for("static_cast_void", Path("src/tests/Nested.cpp")))
        self.assertIsNotNone(config.suppression_for("anonymous_namespace", Path("src/Example.cpp")))
        self.assertIsNotNone(config.suppression_for("oversized_nested_type", Path("src/tests/Nested.cpp")))
        self.assertIsNotNone(config.suppression_for("nested_namespace", Path("src/Example.cpp")))
        self.assertIsNone(config.suppression_for("anonymous_namespace", Path("src/Other.cpp")))

        findings = AntiPatternScanner(config).scan([Path("src/tests/Test.cpp")])
        static_cast_finding = next(
            finding for finding in findings if finding.candidate.rule.identifier == "static_cast_void"
        )
        self.assertTrue(static_cast_finding.suppressed)

    def test_reads_relative_path_pattern_suppression(self) -> None:
        config_path = self.write_config(
            "*[Rule.static_only_class]*\n"
            'Excluded Path: "src/*Traits.hpp"\n'
            'Reason: "Traits are compile-time type mappings."\n'
            "*[Rule.static_only_class]*\n"
            'Excluded Path: "src/Unit?.hpp"\n'
            'Reason: "Single-character pattern."\n'
        )

        config = AntiPatternConfig.read(self.project_directory, config_path)

        self.assertIsNotNone(config.suppression_for("static_only_class", Path("src/ExampleTraits.hpp")))
        self.assertIsNotNone(config.suppression_for("static_only_class", Path("src/Unit1.hpp")))
        self.assertIsNone(config.suppression_for("static_only_class", Path("src/Unit12.hpp")))
        self.assertIsNone(config.suppression_for("static_only_class", Path("src/nested/ExampleTraits.hpp")))
        self.assertIsNone(config.suppression_for("static_only_class", Path("src/Example.hpp")))

    def test_suppresses_native_platform_file_names(self) -> None:
        config_path = self.write_config(
            "*[Rule.regular_string_literal]*\n"
            'Excluded Path: "**/*Windows*"\n'
            'Reason: "Windows native API boundary."\n'
            "*[Rule.regular_string_literal]*\n"
            'Excluded Path: "**/*Posix*"\n'
            'Reason: "POSIX native API boundary."\n'
        )

        config = AntiPatternConfig.read(self.project_directory, config_path)

        self.assertIsNotNone(config.suppression_for("regular_string_literal", Path("src/WindowsPipe.cpp")))
        self.assertIsNotNone(config.suppression_for("regular_string_literal", Path("src/PosixPipe.hpp")))
        self.assertIsNone(config.suppression_for("regular_string_literal", Path("src/PlatformPipe.cpp")))

    def test_scanner_ignores_findings_in_erbsland_os_conditionals(self) -> None:
        path = self.project_directory / "src/Platform.cpp"
        path.write_text(
            'call("outside");\n'
            "#if defined(ERBSLAND_OS_WINDOWS)\n"
            'call("windows");\n'
            "namespace {\n}\n"
            "#endif\n"
            "#if defined(FEATURE)\n"
            'call("feature");\n'
            "#endif\n",
            encoding="utf-8",
        )
        config = AntiPatternConfig.read(self.project_directory, self.write_config())

        findings = AntiPatternScanner(config).scan([Path("src/Platform.cpp")])

        self.assertEqual(2, len(findings))
        self.assertTrue(all(finding.candidate.rule.identifier == "regular_string_literal" for finding in findings))
        self.assertEqual({1, 8}, {finding.line_number for finding in findings})

    def test_rejects_unknown_rule_missing_reason_and_stale_path(self) -> None:
        invalid_configurations = (
            '*[Rule.unknown_rule]*\nExcluded Path: "src"\nReason: "Reason."\n',
            '*[Rule.static_cast_void]*\nExcluded Path: "src"\nReason: ""\n',
            '*[Rule.static_cast_void]*\nExcluded Path: "src/Missing.cpp"\nReason: "Reason."\n',
            '*[Rule.static_cast_void]*\nExcluded Path: "../outside"\nReason: "Reason."\n',
            '*[Rule.static_cast_void]*\nExcluded Path: "../*.cpp"\nReason: "Reason."\n',
            '*[Rule.static_cast_void]*\nExcluded Path: ""\nReason: "Reason."\n',
        )
        for extra in invalid_configurations:
            with self.subTest(extra=extra):
                with self.assertRaises(UtilityError):
                    AntiPatternConfig.read(self.project_directory, self.write_config(extra))

    def test_rejects_duplicate_suppression(self) -> None:
        config_path = self.write_config(
            '*[Rule.static_cast_void]*\nExcluded Path: "src"\nReason: "First."\n'
            '*[Rule.static_cast_void]*\nExcluded Path: "src"\nReason: "Second."\n'
        )

        with self.assertRaises(UtilityError):
            AntiPatternConfig.read(self.project_directory, config_path)


class ReportTest(unittest.TestCase):
    """Tests for stable agent-focused report rendering."""

    def test_sorts_findings_by_severity(self) -> None:
        findings = (
            finding("nested_namespace", Severity.Low, "src/C.cpp", 1),
            finding("regular_string_literal", Severity.Medium, "src/B.cpp", 1),
            finding("oversized_nested_type", Severity.High, "src/A.cpp", 1),
        )

        report = create_report(findings, limit=20, show_suppressed=False)
        finding_lines = [line for line in report.splitlines() if " | example();" in line]

        self.assertIn("HIGH oversized_nested_type", finding_lines[0])
        self.assertIn("MEDIUM regular_string_literal", finding_lines[1])
        self.assertIn("LOW nested_namespace", finding_lines[2])

    def test_groups_missing_documentation_and_sorts_rows_by_count(self) -> None:
        findings = (
            finding("regular_string_literal", Severity.Medium, "src/C.cpp", 1),
            finding("missing_api_documentation", Severity.Medium, "src/B.hpp", 7),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 20),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 5),
            finding("missing_api_documentation", Severity.Medium, "src/B.hpp", 3),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 12),
        )

        report = create_report(findings, limit=20, show_suppressed=False)
        finding_lines = [line for line in report.splitlines() if " | example();" in line]

        self.assertEqual(3, len(finding_lines))
        self.assertIn("src/A.hpp:5 | example(); | 2 more found", finding_lines[0])
        self.assertIn("src/B.hpp:3 | example(); | 1 more found", finding_lines[1])
        self.assertIn("regular_string_literal src/C.cpp:1", finding_lines[2])
        self.assertIn("Summary: 6 active, 0 suppressed, 3 shown (limit 20).", report)

    def test_uses_rule_path_and_line_to_break_count_ties(self) -> None:
        findings = (
            finding("regular_string_literal", Severity.Medium, "src/B.cpp", 1),
            finding("missing_api_documentation", Severity.Medium, "src/C.hpp", 2),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 3),
        )

        report = create_report(findings, limit=20, show_suppressed=False)
        finding_lines = [line for line in report.splitlines() if " | example();" in line]

        self.assertIn("missing_api_documentation src/A.hpp:3", finding_lines[0])
        self.assertIn("missing_api_documentation src/C.hpp:2", finding_lines[1])
        self.assertIn("regular_string_literal src/B.cpp:1", finding_lines[2])

    def test_groups_suppressions_separately_after_filtering(self) -> None:
        accepted_first = Suppression("inline", "Accepted first group.")
        accepted_second = Suppression("inline", "Accepted second group.")
        findings = (
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 10),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 20),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 5, suppression=accepted_first),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 15, suppression=accepted_first),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 25, suppression=accepted_second),
        )

        active_report = create_report(findings, limit=20, show_suppressed=False)
        audit_report = create_report(findings, limit=20, show_suppressed=True)
        active_lines = [line for line in active_report.splitlines() if " | example();" in line]
        audit_lines = [line for line in audit_report.splitlines() if " | example();" in line]

        self.assertEqual(1, len(active_lines))
        self.assertIn("src/A.hpp:10 | example(); | 1 more found", active_lines[0])
        self.assertEqual(3, len(audit_lines))
        self.assertIn("src/A.hpp:5 | example(); | 1 more found | accepted: Accepted first group.", audit_lines[0])
        self.assertIn("src/A.hpp:10 | example(); | 1 more found", audit_lines[1])
        self.assertIn("accepted: Accepted second group.", audit_lines[2])
        self.assertIn("Summary: 2 active, 3 suppressed, 3 shown (limit 20).", audit_report)

    def test_applies_limit_after_grouping(self) -> None:
        findings = (
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 1),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 2),
            finding("missing_api_documentation", Severity.Medium, "src/A.hpp", 3),
            finding("missing_api_documentation", Severity.Medium, "src/B.hpp", 1),
            finding("missing_api_documentation", Severity.Medium, "src/B.hpp", 2),
        )

        report = create_report(findings, limit=1, show_suppressed=False)

        self.assertIn("src/A.hpp:1 | example(); | 2 more found", report)
        self.assertNotIn("src/B.hpp", report)
        self.assertIn("Summary: 5 active, 0 suppressed, 1 shown (limit 1).", report)

    def test_filters_suppressed_findings_and_deduplicates_legend(self) -> None:
        findings = (
            finding("anonymous_namespace", Severity.High, "src/A.cpp", 2),
            finding("anonymous_namespace", Severity.High, "src/B.cpp", 3),
            finding(
                "static_cast_void",
                Severity.Medium,
                "src/C.cpp",
                4,
                suppression=Suppression("configuration", "Accepted centrally."),
            ),
        )

        report = create_report(findings, limit=20, show_suppressed=False)

        self.assertNotIn("src/C.cpp", report)
        self.assertEqual(1, report.count("anonymous_namespace: doc/"))
        self.assertIn("Summary: 2 active, 1 suppressed, 2 shown (limit 20).", report)

    def test_shows_suppressed_reason_and_applies_limit(self) -> None:
        findings = (
            finding("anonymous_namespace", Severity.High, "src/A.cpp", 2),
            finding(
                "static_cast_void",
                Severity.Medium,
                "src/C.cpp",
                4,
                suppression=Suppression("configuration", "Accepted centrally."),
            ),
        )

        report = create_report(findings, limit=1, show_suppressed=True)

        self.assertEqual(1, sum(" | example();" in line for line in report.splitlines()))
        self.assertIn("1 active, 1 suppressed, 1 shown", report)


class AntiPatternsAppTest(unittest.TestCase):
    """Tests for CLI parsing, registration contracts, and exit codes."""

    def test_report_limit_is_bounded(self) -> None:
        self.assertEqual(20, report_limit("20"))
        for value in ("0", "201", "invalid"):
            with self.assertRaises(Exception):
                report_limit(value)

    def test_returns_one_for_active_findings_and_zero_for_only_suppressed(self) -> None:
        config = object()
        active = (finding("anonymous_namespace", Severity.High, "src/A.cpp", 1),)
        suppressed = (
            finding(
                "static_cast_void",
                Severity.Medium,
                "src/B.cpp",
                1,
                suppression=Suppression("configuration", "Accepted."),
            ),
        )
        for findings, expected in ((active, 1), (suppressed, 0)):
            with self.subTest(expected=expected):
                app = AntiPatternsApp()
                with (
                    patch("dev.anti_patterns.AntiPatternConfig.read", return_value=config),
                    patch("dev.anti_patterns.AntiPatternScanner") as scanner_class,
                    redirect_stdout(io.StringIO()),
                ):
                    scanner_class.return_value.scan.return_value = findings
                    self.assertEqual(expected, app.main([]))

    def test_returns_two_for_configuration_or_scanner_failures(self) -> None:
        app = AntiPatternsApp()
        with (
            patch("dev.anti_patterns.AntiPatternConfig.read", side_effect=UtilityError("Invalid configuration.")),
            redirect_stdout(io.StringIO()),
            redirect_stderr(io.StringIO()),
        ):
            self.assertEqual(2, app.main([]))


class AntiPatternConsistencyTest(unittest.TestCase):
    """Ensure stable IDs, documentation filenames, and configured rule sections agree."""

    def test_registered_rules_have_unique_identifiers_and_documentation(self) -> None:
        project_directory = Path(__file__).resolve().parents[2]
        identifiers = [rule.info.identifier for rule in RULES]

        self.assertEqual(len(identifiers), len(set(identifiers)))
        for rule in RULES:
            self.assertRegex(rule.info.identifier, r"^[a-z][a-z0-9_]*$")
            self.assertEqual(f"{rule.info.identifier}.rst", rule.info.documentation_path.name)
            self.assertTrue((project_directory / rule.info.documentation_path).is_file())

    def test_project_configuration_uses_registered_rule_identifiers(self) -> None:
        project_directory = Path(__file__).resolve().parents[2]

        config = AntiPatternConfig.read(project_directory, project_directory / "utilities/conf/anti_patterns.elcl")

        self.assertTrue(config.suppressions)
        self.assertTrue(all(item.rule_identifier in RULES_BY_IDENTIFIER for item in config.suppressions))
        self.assertIsNotNone(config.suppression_for("static_only_class", Path("src/erbsland/unit/ArgumentUnit.hpp")))
        self.assertIsNotNone(config.suppression_for("type_in_wrong_unit", Path("demos/example/Example.cpp")))
        self.assertIsNotNone(config.suppression_for("type_in_wrong_unit", Path("test/unittest/src/Example.cpp")))
        self.assertIsNotNone(config.suppression_for("type_in_wrong_unit", Path("test/profiling/src/Example.cpp")))
        self.assertIsNotNone(config.suppression_for("implementation_in_wrong_unit", Path("demos/Example.cpp")))
        self.assertIsNotNone(
            config.suppression_for("implementation_in_wrong_unit", Path("test/unittest/src/Example.cpp"))
        )
        self.assertIsNotNone(
            config.suppression_for("implementation_in_wrong_unit", Path("src/erbsland/example/WindowsBackend.cpp"))
        )
        self.assertIsNotNone(
            config.suppression_for("implementation_in_wrong_unit", Path("src/erbsland/example/PosixBackend.cpp"))
        )
        self.assertIsNone(
            config.suppression_for("implementation_in_wrong_unit", Path("src/erbsland/example/PlatformBackend.cpp"))
        )


if __name__ == "__main__":
    unittest.main()
