// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/headers/HeaderDeclarations.hpp>
#include <tool/headers/PublicHeadersTool.hpp>

#include <filesystem>

TESTED_TARGETS(HeaderPolicy HeaderDeclarations PublicHeaders PublicHeadersTool)
class PublicHeadersTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testDirectoryLocalUmbrella() {
        configure();
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_FALSE(read("src/sample/net/all.hpp"_el).contains("tls/Connection.hpp"_el));
        REQUIRE(read("src/sample/net/tls/all.hpp"_el).contains("Connection.hpp"_el));
        REQUIRE((_root / "include/sample/all_net_tls.hpp"_el).info().exists());
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testParallelGenerationMatchesSingleWorkerBytes() {
        configure();
        for (auto index = 0U; index < 80U; ++index) {
            write(
                el::StringFormat{"src/sample/net/Type{}.hpp"_el}.build(index),
                el::StringFormat{"namespace sample::net {{ class Type{} {{}}; }}\n"_el}.build(index));
        }
        REQUIRE_EQUAL(run(el::StringList{"--jobs"_el, "1"_el}), 0);
        const auto expected = snapshot();
        for (const auto &workers : el::StringList{"2"_el, "4"_el}) {
            (_root / "include"_el).operations().removeOrThrow(el::PathRemoveOptions{}.setRecursive(true));
            REQUIRE_EQUAL(run(el::StringList{"--jobs"_el, workers}), 0);
            const auto actual = snapshot();
            REQUIRE_EQUAL(actual.count(), expected.count());
            for (const auto &[path, bytes] : expected) {
                REQUIRE_EQUAL(actual.get(path).value(), bytes);
            }
            REQUIRE_EQUAL(run(el::StringList{"--jobs"_el, workers, "--check"_el}), 0);
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testWorkerFailuresPreserveGeneratedFiles() {
        configure();
        REQUIRE_EQUAL(run(), 0);
        const auto mapping = read("map.txt"_el);
        const auto wrapper = read("include/sample/Connection.hpp"_el);
        write("src/sample/net/tls/Connection.hpp"_el, "/* unfinished"_el);
        REQUIRE_EQUAL(run(el::StringList{"--jobs"_el, "4"_el}), 1);
        REQUIRE_EQUAL(read("map.txt"_el), mapping);
        REQUIRE_EQUAL(read("include/sample/Connection.hpp"_el), wrapper);
        write("src/sample/net/tls/Connection.hpp"_el, "namespace sample::net { class Connection {}; }\n"_el);
        write("src/sample/net/tls/Connection_fwd.hpp"_el, "/* unfinished"_el);
        REQUIRE_EQUAL(run(el::StringList{"--jobs"_el, "4"_el}), 1);
        REQUIRE_EQUAL(read("map.txt"_el), mapping);
        REQUIRE_EQUAL(read("include/sample/Connection.hpp"_el), wrapper);
        (_root / "src/sample/net/tls/Connection_fwd.hpp"_el).operations().removeOrThrow();
        (_root / "src/sample/net/BadEncoding.hpp"_el).content().writeDataOrThrow(el::ByteBlock{0xffU, 0xfeU});
        REQUIRE_EQUAL(run(el::StringList{"--jobs"_el, "4"_el}), 1);
        REQUIRE_EQUAL(read("map.txt"_el), mapping);
        REQUIRE_EQUAL(read("include/sample/Connection.hpp"_el), wrapper);
    }

    void testInvalidWorkerCounts() {
        configure();
        REQUIRE_EQUAL(run(el::StringList{"--jobs"_el, "0"_el}), 1);
        REQUIRE_EQUAL(run(el::StringList{"--jobs"_el, "65"_el}), 1);
        REQUIRE_FALSE((_root / "map.txt"_el).info().exists());
    }

    void testDeclarationScannerLight() {
        const auto scanner =
            tool::headers::HeaderDeclarations{"// namespace wrong { class Fake {}; }\n"
                                              "namespace sample { namespace net {\n"
                                              "const auto text = R\"tag(namespace fake { class Fake {}; })tag\";\n"
                                              "class Connection { class Nested {}; };\n"
                                              "enum class State : std::uint8_t { Ready };\n"
                                              "using ConnectionPtr = Ptr<Connection>;\n"
                                              "template<class T> class Generic {};\n"
                                              "enum struct Status : int { Ready };\n"
                                              "extern template class Generic<int>;\n"
                                              "}}\n"_el};
        REQUIRE(scanner.namespaces().contains("sample::net"_el));
        REQUIRE_FALSE(scanner.namespaces().contains("wrong"_el));
        REQUIRE_FALSE(scanner.namespaces().contains("sample::net::fake"_el));
        REQUIRE_EQUAL(scanner.declarations().size(), std::size_t{6});
        REQUIRE_EQUAL(scanner.declarations()[0].forwardText, "class Connection;"_el);
        REQUIRE_EQUAL(scanner.declarations()[1].forwardText, "enum class State : std::uint8_t;"_el);
        REQUIRE(scanner.declarations()[2].forwardText.isEmpty());
        REQUIRE(scanner.declarations()[3].forwardText.isEmpty());
        REQUIRE_EQUAL(scanner.declarations()[4].forwardText, "enum struct Status : int;"_el);
        REQUIRE(scanner.declarations()[5].forwardText.isEmpty());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testGenerationAndCheck() {
        configure();
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 1);
        REQUIRE_FALSE((_root / "include/sample/Connection.hpp"_el).info().exists());
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
        const auto wrapper = read("include/sample/Connection.hpp"_el);
        REQUIRE(wrapper.contains("src/sample/net/tls/Connection.hpp"_el));
        REQUIRE(read("include/sample/net/tls/Connection.hpp"_el).contains("Connection.hpp"_el));
        REQUIRE_FALSE(read("src/sample/net/all.hpp"_el).contains("tls/Connection.hpp"_el));
        REQUIRE_FALSE(read("src/sample/net/all.hpp"_el).contains("StdFormat.hpp"_el));
        REQUIRE_FALSE((_root / "include/sample/all.hpp"_el).info().exists());
        REQUIRE(read("map.txt"_el)
                .contains("net/tls/Connection.hpp;Connection.hpp\nnet/tls/Connection.hpp;net/tls/Connection.hpp\n"_el));
        const auto timestamp =
            std::filesystem::last_write_time((_root / "include/sample/Connection.hpp"_el).toStdPath());
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_EQUAL(
            std::filesystem::last_write_time((_root / "include/sample/Connection.hpp"_el).toStdPath()), timestamp);
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
        const auto mapping = read("map.txt"_el).replacedAll("\n"_el, "\r\n"_el);
        write("map.txt"_el, mapping);
        const auto mappingTimestamp = std::filesystem::last_write_time((_root / "map.txt"_el).toStdPath());
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_EQUAL(read("map.txt"_el), mapping);
        REQUIRE_EQUAL(std::filesystem::last_write_time((_root / "map.txt"_el).toStdPath()), mappingTimestamp);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCoverageAndNamespaceErrorsPreserveOutputs() {
        configure();
        REQUIRE_EQUAL(run(), 0);
        const auto mapping = read("map.txt"_el);
        write("src/sample/new/Example.hpp"_el, "namespace sample::new_domain { class Example {}; }\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_EQUAL(read("map.txt"_el), mapping);
        (_root / "src/sample/new"_el).operations().removeOrThrow(el::PathRemoveOptions{}.setRecursive(true));
        write("src/sample/net/tls/Connection.hpp"_el, "namespace sample::wrong { class Connection {}; }\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_EQUAL(read("map.txt"_el), mapping);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNestedNamespacesAndSelectiveImports() {
        configure(
            "[update_public_headers.directory.net.tls]\nnested_namespace: Yes\nflatten: No\ntarget_directory: \"tls\"\n"
            "[update_public_headers.directory.re]\nflatten: Yes\nincluded_symbols: \"RegEx\"\n"_el,
            "namespace sample::net::tls { class Connection {}; }\n"_el);
        write(
            "src/sample/re/RegEx.hpp"_el, "#include \"RegEx_fwd.hpp\"\nnamespace sample::re { class RegEx {}; }\n"_el);
        write("src/sample/re/RegEx_fwd.hpp"_el, "namespace sample::re { class RegEx; using RegExPtr = RegEx*; }\n"_el);
        write("src/sample/re/Other.hpp"_el, "namespace sample::re { class Other {}; }\n"_el);
        REQUIRE_EQUAL(run(), 0);
        const auto exports = read("src/sample/core/impl/MakeOneNamespace.hpp"_el);
        REQUIRE(exports.contains("RegEx_fwd.hpp"_el));
        REQUIRE(exports.contains("using ::sample::re::RegEx;"_el));
        REQUIRE_FALSE(exports.contains("using ::sample::re::RegExPtr;"_el));
        REQUIRE_FALSE(exports.contains("using namespace ::sample::re;"_el));
        REQUIRE(read("map.txt"_el).contains("net/tls/Connection.hpp;tls/Connection.hpp\n"_el));
        REQUIRE(read("map.txt"_el).contains("net/tls/all.hpp;tls/all.hpp\n"_el));
        REQUIRE_FALSE((_root / "include/sample/Other.hpp"_el).info().exists());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testExtractionMergingAndStaleOutputs() {
        configure(
            "[update_public_headers.directory.net.tls]\n[update_public_headers.directory.other]\nflatten: Yes\nincluded_symbols: \"Extra\"\n"_el);
        write("src/sample/other/Extra.hpp"_el, "namespace sample::other { class Extra {}; }\n"_el);
        write("src/sample/net/Literals.hpp"_el, "namespace sample::net {}\n"_el);
        write("src/sample/other/Literals.hpp"_el, "namespace sample::other {}\n"_el);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE(read("src/sample/core/impl/MakeOneNamespace.hpp"_el).contains("class Extra;"_el));
        write(
            "include/sample/Stale.hpp"_el,
            "// !!! THIS IS A GENERATED FILE - DO NOT EDIT MANUALLY !!!\n// update_includes.py\n"_el);
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 1);
        REQUIRE((_root / "include/sample/Stale.hpp"_el).info().exists());
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_FALSE((_root / "include/sample/Stale.hpp"_el).info().exists());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testConfiguredAliasRecovery() {
        configure(
            "[update_public_headers.directory.net.tls]\n"
            "[update_public_headers.header_aliases.connection]\n"
            "source_header: \"net/tls/Connection.hpp\"\ninclude_header: \"LegacyConnection.hpp\"\n"_el);
        REQUIRE_EQUAL(run(), 0);
        const auto alias = _root / "include/sample/LegacyConnection.hpp"_el;
        REQUIRE(alias.info().exists());
        alias.operations().removeOrThrow();
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 1);
        REQUIRE_FALSE(alias.info().exists());
        REQUIRE_EQUAL(run(), 0);
        alias.info().reload();
        REQUIRE(alias.info().exists());
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFilenameGlobsAndConflicts() {
        configure("[update_public_headers.directory.net.tls]\nincluded_headers: \"sub/*.hpp\"\n"_el);
        REQUIRE_EQUAL(run(), 1);
        configure(
            "[update_public_headers.directory.net.tls]\n[update_public_headers.directory.other]\nflatten: Yes\n"_el);
        write("src/sample/other/Connection.hpp"_el, "namespace sample::other { class Connection {}; }\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_FALSE((_root / "map.txt"_el).info().exists());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testExplicitMergesAliasesAndExclusions() {
        configure(
            "[update_public_headers.directory.net.tls]\n"
            "[update_public_headers.directory.other]\nflatten: Yes\n"
            "excluded_headers: \"Private.hpp\"\nexcluded_from_flattening: \"Local.hpp\"\n"
            "target_namespace: \"extra\"\n"_el);
        write("src/sample/net/Literals.hpp"_el, "namespace sample::net {}\n"_el);
        write("src/sample/other/Literals.hpp"_el, "namespace sample::other {}\n"_el);
        write("src/sample/other/Local.hpp"_el, "namespace sample::other { class Local {}; }\n"_el);
        write("src/sample/other/Private.hpp"_el, "namespace unrelated {}\n"_el);
        write("src/sample/other/impl/Hidden.hpp"_el, "namespace unrelated {}\n"_el);
        REQUIRE_EQUAL(run(), 0);
        const auto merged = read("include/sample/Literals.hpp"_el);
        REQUIRE(merged.contains("net/Literals.hpp"_el));
        REQUIRE(merged.contains("other/Literals.hpp"_el));
        const auto mapping = read("map.txt"_el);
        REQUIRE(mapping.contains("net/Literals.hpp;Literals.hpp\n"_el));
        REQUIRE(mapping.contains("other/Literals.hpp;Literals.hpp\n"_el));
        REQUIRE(mapping.contains("other/Local.hpp;other/Local.hpp\n"_el));
        REQUIRE_FALSE(mapping.contains("Private.hpp"_el));
        REQUIRE_FALSE(mapping.contains("Hidden.hpp"_el));
        REQUIRE(read("src/sample/core/impl/MakeOneNamespace.hpp"_el).contains("namespace extra = ::sample::other;"_el));
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testUnsupportedDeclarationsAndForwardNamespaces() {
        configure(
            "[update_public_headers.directory.net.tls]\nincluded_symbols: \"Connection\"\n"_el,
            "namespace sample::net { template<class T> class Connection {}; }\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_FALSE((_root / "map.txt"_el).info().exists());
        write(
            "src/sample/net/tls/Connection_fwd.hpp"_el,
            "namespace sample::wrong { template<class T> class Connection; }\n"_el);
        REQUIRE_EQUAL(run(), 1);
        write(
            "src/sample/net/tls/Connection_fwd.hpp"_el,
            "namespace sample { namespace net { template<class T> class Connection; }}\n"_el);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE(read("src/sample/core/impl/MakeOneNamespace.hpp"_el).contains("Connection_fwd.hpp"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPortableDirectoryConflictsAndLegacyRoots() {
        configure(
            "[update_public_headers.directory.net.tls]\ntarget_directory: \"Net\"\n"
            "[update_public_headers.directory.other]\ntarget_directory: \"net\"\n"_el);
        write("src/sample/other/Extra.hpp"_el, "namespace sample::other { class Extra {}; }\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_FALSE((_root / "map.txt"_el).info().exists());
        configure("[update_public_headers.directory.net.tls]\n[update_public_headers.directory.other]\n"_el);
        const auto legacy = "// !!! THIS IS A GENERATED FILE - DO NOT EDIT MANUALLY !!!\n// update_includes.py\n"_el;
        write("src/sample/all.hpp"_el, legacy);
        write("include/sample/all.hpp"_el, legacy);
        write("include/sample/Manual.hpp"_el, "// A user-managed header.\n"_el);
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 1);
        REQUIRE((_root / "src/sample/all.hpp"_el).info().exists());
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_FALSE((_root / "src/sample/all.hpp"_el).info().exists());
        REQUIRE_FALSE((_root / "include/sample/all.hpp"_el).info().exists());
        REQUIRE((_root / "include/sample/Manual.hpp"_el).info().exists());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPlannedHeaderCannotBeAnotherOutputsParent() {
        configure(
            "[update_public_headers.directory.net.tls]\n"
            "[update_public_headers.header_aliases.connection]\n"
            "source_header: \"net/tls/Connection.hpp\"\n"
            "include_header: \"Connection.hpp/Child.hpp\"\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_FALSE((_root / "map.txt"_el).info().exists());
        REQUIRE_FALSE((_root / "include/sample/Connection.hpp"_el).info().exists());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testTypedDirectoryRulesAndInheritance() {
        configure("[update_public_headers.directory.net.tls]\nflatten: \"wrong\"\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_FALSE((_root / "map.txt"_el).info().exists());
        configure("[update_public_headers.directory.net.tls]\nincluded_headers: \"*.hpp\", 42\n"_el);
        REQUIRE_EQUAL(run(), 1);
        configure(
            "[update_public_headers.directory.net.tls]\n"
            "[update_public_headers.directory.net.tls.extra.more]\nflatten: Yes\n"_el);
        write("src/sample/net/tls/extra/more/Deep.hpp"_el, "namespace sample::net { class Deep {}; }\n"_el);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE(read("map.txt"_el).contains("net/tls/extra/more/Deep.hpp;Deep.hpp"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testConfiguredMarkersAndCleanupBoundaries() {
        configure();
        auto configuration = read("tool/conf/erbsland-core-tool.elcl"_el);
        configuration = configuration.replacedAll(
            "[update_public_headers]"_el,
            "[update_public_headers]\ngenerated_markers: \"// GENERATED PUBLIC HEADER\", \"// !!! THIS IS A GENERATED FILE\""_el);
        write("tool/conf/erbsland-core-tool.elcl"_el, configuration);
        write("include/sample/Stale.hpp"_el, "// GENERATED PUBLIC HEADER\n"_el);
        write("include/sample/TenthLine.hpp"_el, "\n\n\n\n\n\n\n\n\n// GENERATED PUBLIC HEADER\n"_el);
        write("include/sample/EleventhLine.hpp"_el, "\n\n\n\n\n\n\n\n\n\n// GENERATED PUBLIC HEADER\n"_el);
        write("src/sample/core/impl/OtherData.hpp"_el, "// !!! THIS IS A GENERATED FILE\n"_el);
        write("include/sample/Handwritten.hpp"_el, "const auto text = R\"(\n// GENERATED PUBLIC HEADER\n)\";\n"_el);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE(read("include/sample/Connection.hpp"_el).contains("// GENERATED PUBLIC HEADER"_el));
        REQUIRE_FALSE((_root / "include/sample/Stale.hpp"_el).info().exists());
        REQUIRE_FALSE((_root / "include/sample/TenthLine.hpp"_el).info().exists());
        REQUIRE((_root / "include/sample/EleventhLine.hpp"_el).info().exists());
        REQUIRE((_root / "src/sample/core/impl/OtherData.hpp"_el).info().exists());
        REQUIRE((_root / "include/sample/Handwritten.hpp"_el).info().exists());
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
    }

private:
    /// Capture exact project bytes to compare serial and parallel generation.
    [[nodiscard]] auto snapshot() -> el::StringMap<el::ByteBlock> {
        auto result = el::StringMap<el::ByteBlock>{};
        _root.walker().walkOrThrow(
            [&](const el::Path &path) -> el::PathWalkStatus {
                result.set(path.toRelativeOrThrow(_root).toString(), files().readData(path));
                return el::PathWalkStatus::Continue;
            },
            el::PathWalkOptions{}.setTypes(el::PathType::RegularFile));
        return result;
    }

    void configure(
        const el::String &sections = "[update_public_headers.directory.net.tls]\n"_el,
        const el::String &connection = "namespace sample::net { class Connection {}; }\n"_el) {
        write("src/sample/core/Definitions.hpp"_el, "#pragma once\n"_el);
        write("src/sample/net/tls/Connection.hpp"_el, connection);
        write("src/sample/net/StdFormat.hpp"_el, "namespace std {}\n"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            el::join(
                "[rendering]\n[update_public_headers]\n"
                "source_base_directory: \"src/sample\"\ninclude_base_directory: \"include/sample\"\n"
                "base_namespace: \"sample\"\nmapping_file: \"map.txt\"\n"
                "excluded_directories: \"impl\"\nexcluded_headers: \"*_*.hpp\"\n"
                "mergeable_header_names: \"Literals.hpp\"\n"
                "[update_public_headers.directory.core]\nflatten: Yes\n"
                "[update_public_headers.directory.net]\nflatten: Yes\nexcluded_from_all_headers: \"StdFormat.hpp\"\n"_el,
                sections));
    }

    auto run(const el::StringList &options = {}) -> int {
        return runTool<tool::headers::PublicHeadersTool>({}, {}, "update-public-headers"_el, options);
    }

    auto read(const el::String &path) -> el::String { return files().readText(el::Path{path}); }
};
