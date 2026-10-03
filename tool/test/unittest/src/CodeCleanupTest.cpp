// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/cleanup/CleanupText.hpp>
#include <tool/cleanup/CMakeCleanup.hpp>
#include <tool/cleanup/CppCleanup.hpp>
#include <tool/cleanup/CppSpacing.hpp>

TESTED_TARGETS(
    tool::cleanup::CppCleanup tool::cleanup::CppSpacing tool::cleanup::IncludeIndex tool::cleanup::CMakeCleanup)
class CodeCleanupTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testConfiguredLibraryAndPublicIncludePrefixes() {
        auto policy = makePolicy();
        policy.library = _root / "vendor/base/source"_el;
        policy.publicIncludePrefix = el::Path{"vendor/base"_el};
        policy.trees[0].directory = policy.library;
        policy.trees[0].includeRoots = el::PathList{policy.library};
        auto index = makeIndex(policy);
        const auto cpp = tool::cleanup::CppCleanup{index};
        const auto source =
            "#include \"../../vendor/base/source/a/Widget.hpp\"\n#include <vendor/base/a/Widget.hpp>\n"_el;
        const auto result = cpp.apply(_root / "consumer/test/Example.cpp"_el, source, policy.trees[1]);
        REQUIRE_EQUAL(result, "#include <vendor/base/Widget.hpp>\n"_el);
        REQUIRE_EQUAL(cpp.apply(_root / "consumer/test/Example.cpp"_el, result, policy.trees[1]), result);
        auto global = true;
        REQUIRE_EQUAL(
            index.resolve(policy.library / "a/File.cpp"_el, "vendor/base/Widget.hpp"_el, global, policy.trees[0]),
            "Widget.hpp"_el);
        REQUIRE_FALSE(global);
        global = true;
        REQUIRE_EQUAL(
            index.resolve(_root / "consumer/test/Example.cpp"_el, "erbsland/Other.hpp"_el, global, policy.trees[1]),
            "erbsland/Other.hpp"_el);
        REQUIRE_EQUAL(
            index.resolve(
                _root / "consumer/test/Example.cpp"_el, "unrelated/src/External.h"_el, global, policy.trees[1]),
            "unrelated/src/External.h"_el);
    }

    void testSpacingAfterMultibyteCommentsAndMultilineLiterals() {
        const auto source =
            "// ä😀\r\nnamespace 名 {\r\nconst auto text = R\"raw(😀\r\nnamespace fake {\r\n)raw\";\r\nvoid one() {\r\n}\r\nvoid two() {\r\n}\r\n}\r\n"_el;
        const auto result = tool::cleanup::CppSpacing::apply(source, true);
        REQUIRE(result.startsWith("// ä😀\n\nnamespace 名 {\n\n"_el));
        REQUIRE(result.contains("R\"raw(😀\nnamespace fake {\n)raw\";"_el));
        REQUIRE(result.contains("}\n\nvoid two()"_el));
        REQUIRE(result.endsWith("}\n\n}\n"_el));
        REQUIRE_EQUAL(tool::cleanup::CppSpacing::apply(result, true), result);
    }

    void testIncludesCanonicalizeSortAndPreserveComments() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        const auto cpp = tool::cleanup::CppCleanup{index};
        const auto source = "// Copyright (c) 2026 Test\n// SPDX-License-Identifier: Apache-2.0\n\n"
                            "#include <vector>\n#include \"../../src/erbsland/a/Widget.hpp\" // needed\n"
                            "#include \"Local.hpp\"\n#include <vector>\n#include HEADER\n"
                            "#include <string>\n"_el;
        const auto result = cpp.apply(_root / "consumer/test/Example.cpp"_el, source, policy.trees[1]);
        REQUIRE(result.contains("#include <erbsland/Widget.hpp> // needed"_el));
        REQUIRE(result.contains("#include HEADER\n#include <string>"_el));
        REQUIRE(result.contains("#include \"Local.hpp\"\n\n#include <erbsland/Widget.hpp>"_el));
        REQUIRE(result.find("#include <vector>"_el, result.find("#include <vector>"_el).incremented()).isNoIndex());
        REQUIRE_EQUAL(cpp.apply(_root / "consumer/test/Example.cpp"_el, result, policy.trees[1]), result);
    }

    void testLibraryPathsAndMainHeader() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        const auto cpp = tool::cleanup::CppCleanup{index};
        const auto source = "// Copyright (c) 2026 Test\n// SPDX-License-Identifier: Apache-2.0\n\n"
                            "#include <vector>\n#include \"src/erbsland/a/Widget.hpp\"\n#include \"b/Other.hpp\"\n"_el;
        const auto result = cpp.apply(policy.library / "a/Widget_part.cpp"_el, source, policy.trees[0]);
        REQUIRE(result.contains("Apache-2.0\n#include \"Widget.hpp\"\n\n#include \"../b/Other.hpp\""_el));
    }

    void testLocalTemplateMergedAndInternalHeaders() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        auto global = false;
        const auto file = _root / "consumer/test/Example.cpp"_el;
        const auto &tree = policy.trees[1];
        REQUIRE_EQUAL(index.resolve(file, "../src/Worker.hpp"_el, global, tree), "../src/Worker.hpp"_el);
        REQUIRE(!global);
        REQUIRE_EQUAL(index.resolve(file, "Generated.hpp"_el, global, tree), "Generated.hpp"_el);
        REQUIRE_EQUAL(index.resolve(file, "erbsland/Literals.hpp"_el, global, tree), "erbsland/Literals.hpp"_el);
        REQUIRE(global);
        REQUIRE_EQUAL(
            index.resolve(file, "erbsland/a/impl/Private.hpp"_el, global, tree), "erbsland/a/impl/Private.hpp"_el);
    }

    void testAmbiguityAndMissingHeaders() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        index.add(policy.library / "b/Widget.hpp"_el);
        auto global = false;
        const auto file = _root / "consumer/test/Example.cpp"_el;
        REQUIRE_THROWS_AS(el::ApplicationError, index.resolve(file, "Widget.hpp"_el, global, policy.trees[1]));
        REQUIRE_THROWS_AS(el::ApplicationError, index.resolve(file, "Missing.hpp"_el, global, policy.trees[1]));
        REQUIRE_EQUAL(index.resolve(file, "a/Widget.hpp"_el, global, policy.trees[1]), "erbsland/Widget.hpp"_el);
    }

    void testPragmaNamespaceAndFunctionSpacing() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        const auto cpp = tool::cleanup::CppCleanup{index};
        const auto source =
            "// Copyright (c) 2026 Test\n// SPDX-License-Identifier: Apache-2.0\n\n#pragma once\nnamespace demo {\nclass Example {};\n}\n"_el;
        REQUIRE_EQUAL(
            cpp.apply(policy.library / "a/Example.hpp"_el, source, policy.trees[0]),
            "// Copyright (c) 2026 Test\n// SPDX-License-Identifier: Apache-2.0\n#pragma once\n\nnamespace demo {\n\nclass Example {};\n\n}\n"_el);
        const auto functions = "namespace demo {\nvoid one() {\n}\nvoid two() {\n}\n}\n"_el;
        REQUIRE_EQUAL(
            tool::cleanup::CppSpacing::apply(functions, true),
            "namespace demo {\n\nvoid one() {\n}\n\nvoid two() {\n}\n\n}\n"_el);
    }

    void testCopyrightSpellingAndYearsRemainUnchanged() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        const auto cpp = tool::cleanup::CppCleanup{index};
        for (const auto &notice : {"// Copyright © 2001-2020 Owner"_el, "// Copyright (c) 2001-2020 Owner"_el}) {
            const auto source =
                el::join(notice, "\n// SPDX-License-Identifier: Apache-2.0\n#pragma once\n\n#include <vector>\n"_el);
            REQUIRE_EQUAL(cpp.apply(policy.library / "a/Example.hpp"_el, source, policy.trees[0]), source);
        }
    }

    void testTemplateDefaultArgumentAndSplitNamespaceSpacing() {
        const auto source = "namespace demo\n{\ntemplate<class T>\nvoid one(int n = 0) {\n}\nvoid two() {\n}\n}\n"_el;
        const auto output = tool::cleanup::CppSpacing::apply(source, true);
        REQUIRE(output.contains("namespace demo\n{\n\ntemplate"_el));
        REQUIRE(output.contains("}\n\nvoid two()"_el));
        REQUIRE_EQUAL(tool::cleanup::CppSpacing::apply(output, true), output);
    }

    void testLexicalSpacingPreservesRawStringsAndDirectives() {
        const auto source = "namespace demo {\n\nconst auto text = R\"raw(\n}\nnamespace fake {\n)raw\";\n"
                            "#define BODY \\\n  namespace ignored { }\n\n}\n"_el;
        REQUIRE_EQUAL(tool::cleanup::CppSpacing::apply(source, true), source);
    }

    void testIncludeOnlyFilesRemainStable() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        const auto cpp = tool::cleanup::CppCleanup{index};
        const auto source = "// Copyright\n#pragma once\n\n#include <vector>\n"_el;
        const auto result = cpp.apply(policy.library / "a/Header.hpp"_el, source, policy.trees[0]);
        REQUIRE(!result.endsWith("\n\n"_el));
        REQUIRE_EQUAL(cpp.apply(policy.library / "a/Header.hpp"_el, result, policy.trees[0]), result);
        REQUIRE_EQUAL(
            tool::cleanup::CppCleanup::codeFingerprint(source), tool::cleanup::CppCleanup::codeFingerprint(result));
    }

    void testFingerprintsIgnoreIncludeEditsButDetectCode() {
        const auto one = "// Header\n#include <vector>\n\nvoid one() {}\n"_el;
        const auto two = "// Header\n#include <string> // changed include\n#include <vector>\n\nvoid one() {}\n"_el;
        REQUIRE_EQUAL(tool::cleanup::CppCleanup::codeFingerprint(one), tool::cleanup::CppCleanup::codeFingerprint(two));
        REQUIRE_NOT_EQUAL(
            tool::cleanup::CppCleanup::codeFingerprint(one),
            tool::cleanup::CppCleanup::codeFingerprint("void two() {}\n"_el));
    }

    void testGeneratedCommentBoundariesRemainStable() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        const auto cpp = tool::cleanup::CppCleanup{index};
        for (const auto &gap : {""_el, "\n"_el}) {
            const auto source = el::join(
                "// Copyright\n// SPDX-License-Identifier: Apache-2.0\n"_el,
                gap,
                "// Generated file; do not edit.\n"_el,
                gap,
                "#include \"Widget.hpp\"\n\nnamespace demo {\n\nint value;\n\n}\n"_el);
            REQUIRE_EQUAL(cpp.apply(policy.library / "a/Widget.cpp"_el, source, policy.trees[0]), source);
        }
    }

    void testDocumentationAndManualFormattingBoundaries() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        const auto cpp = tool::cleanup::CppCleanup{index};
        const auto documented = "#pragma once\n\n#include <vector>\n\n/// An example type.\nclass Example {};\n"_el;
        REQUIRE_EQUAL(cpp.apply(policy.library / "a/Example.hpp"_el, documented, policy.trees[0]), documented);
        const auto documentedNamespace =
            "#pragma once\n\n/// An example namespace.\nnamespace demo {\n\nint value;\n\n}\n"_el;
        REQUIRE_EQUAL(
            cpp.apply(policy.library / "a/Example.hpp"_el, documentedNamespace, policy.trees[0]), documentedNamespace);
        const auto manual =
            "#pragma once\n\n// clang-format off\nnamespace one {}\nnamespace two {}\n// clang-format on\n"_el;
        REQUIRE_EQUAL(cpp.apply(policy.library / "a/Example.hpp"_el, manual, policy.trees[0]), manual);
        REQUIRE_EQUAL(
            tool::cleanup::CppSpacing::apply("namespace one {}\nnamespace two {}\n"_el, false),
            "namespace one {}\nnamespace two {}\n"_el);
    }

    void testNamespaceSpacingBeforeTrailingTemplateInclude() {
        const auto text = "namespace demo {\n\nclass Example {};\n}\n\n#include \"Example.tpp\"\n"_el;
        REQUIRE_EQUAL(
            tool::cleanup::CppSpacing::apply(text, false),
            "namespace demo {\n\nclass Example {};\n\n}\n\n#include \"Example.tpp\"\n"_el);
    }

    void testCMakeAddsLocalFilesAndKeepsConditionalSources() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        policy.cmake.emplace_back(policy.library, "example"_el);
        const auto directory = policy.library / "a"_el;
        index.add(directory / "Conditional.cpp"_el);
        const auto cleaner = tool::cleanup::CMakeCleanup{policy, index};
        const auto result = cleaner.apply(
            directory / "CMakeLists.txt"_el,
            "target_sources(example PRIVATE Widget.hpp)\nif(WIN32)\n    target_sources(example PRIVATE Conditional.cpp)\nendif()\n"_el,
            el::StringList{"Widget.hpp"_el, "Conditional.cpp"_el},
            "# Copyright\n# License"_el);
        REQUIRE(result.has_value());
        REQUIRE(result->contains("target_sources(example PRIVATE\n        Widget.hpp\n)\n\nif(WIN32)"_el));
        REQUIRE(result->contains("    target_sources(example PRIVATE\n            Conditional.cpp\n    )"_el));
    }

    void testCMakeRejectsMissingAndNonlocalSources() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        policy.cmake.emplace_back(policy.library, "example"_el);
        const auto cleaner = tool::cleanup::CMakeCleanup{policy, index};
        for (
            const auto &source :
            {"target_sources(example PRIVATE Missing.cpp)"_el, "target_sources(example PRIVATE ../b/Other.hpp)"_el}) {
            REQUIRE_THROWS_AS(
                el::ApplicationError,
                cleaner.apply(policy.library / "a/CMakeLists.txt"_el, source, {}, "# Copyright"_el));
        }
    }

    void testCMakePreservesUnsupportedSemantics() {
        auto policy = makePolicy();
        auto index = makeIndex(policy);
        policy.cmake.emplace_back(policy.library, "example"_el);
        const auto cleaner = tool::cleanup::CMakeCleanup{policy, index};
        for (
            const auto &source :
            {"target_sources(example PUBLIC Widget.hpp)\n"_el,
                "target_sources(example PRIVATE FILE_SET headers TYPE HEADERS FILES Widget.hpp)\n"_el,
                "add_subdirectory(child EXCLUDE_FROM_ALL)\n"_el,
                "target_sources(example PRIVATE Missing.cpp)\nset(extra other)\n"_el,
                "cmake_minimum_required(VERSION 3.20...3.28)\n"_el,
                "set(files Widget.hpp)\ntarget_sources(example PRIVATE ${files})\n"_el}) {
            REQUIRE(!cleaner.apply(policy.library / "a/CMakeLists.txt"_el, source, {}, "# Copyright"_el).has_value());
        }
    }

private:
    auto makePolicy() const -> tool::cleanup::CleanupPolicy {
        auto policy = tool::cleanup::CleanupPolicy{};
        policy.library = _root / "src/erbsland"_el;
        policy.publicIncludePrefix = el::Path{"erbsland"_el};
        policy.cmakeVersion = "3.28"_el;
        policy.trees.push_back({policy.library, false, el::PathList{policy.library}});
        policy.trees.push_back({_root / "consumer"_el, true, el::PathList{_root / "consumer"_el}});
        return policy;
    }

    auto makeIndex(const tool::cleanup::CleanupPolicy &policy) const -> tool::cleanup::IncludeIndex {
        auto index = tool::cleanup::IncludeIndex{policy, _root};
        for (const auto &name : {"a/Widget.hpp"_el, "b/Other.hpp"_el, "a/impl/Private.hpp"_el}) {
            index.add(policy.library / name);
        }
        for (
            const auto &name :
            {"consumer/test/Local.hpp"_el, "consumer/src/Worker.hpp"_el, "consumer/test/Generated.hpp.in"_el}) {
            index.add(_root / name);
        }
        index.readMapping(
            "a/Widget.hpp;Widget.hpp\na/Widget.hpp;a/Widget.hpp\ntext/Literals.hpp;Literals.hpp\ntime/Literals.hpp;Literals.hpp\n"_el);
        return index;
    }
};
