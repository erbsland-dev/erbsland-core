// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/headers/PublicHeadersTool.hpp>

TESTED_TARGETS(HeaderPolicy PublicHeaders PublicHeadersTool)
class PublicHeaderPolicyTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testDirectMembershipAcrossDirectoryKinds() {
        configure(
            "[update_public_headers.directory.net]\nflatten: Yes\n"
            "[update_public_headers.directory.net.tls]\n"
            "[update_public_headers.directory.net.tls.deep]\n"
            "[update_public_headers.directory.net.tls_extra]\n"
            "[update_public_headers.directory.net.zip]\nnested_namespace: Yes\nflatten: No\ntarget_directory: \"zip\"\n"
            "[update_public_headers.directory.empty]\n"_el);
        write("src/sample/net/Main.hpp"_el, "namespace sample::net { class Main {}; }\n"_el);
        write("src/sample/net/tls/Tls.hpp"_el, "namespace sample::net { class Tls {}; }\n"_el);
        write("src/sample/net/tls/deep/Deep.hpp"_el, "namespace sample::net { class Deep {}; }\n"_el);
        write("src/sample/net/tls_extra/Extra.hpp"_el, "namespace sample::net { class Extra {}; }\n"_el);
        write("src/sample/net/zip/Zip.hpp"_el, "namespace sample::net::zip { class Zip {}; }\n"_el);
        (_root / "src/sample/empty"_el).operations().createDirectoryOrThrow();
        REQUIRE_EQUAL(run(), 0);
        const auto main = read("src/sample/net/all.hpp"_el);
        REQUIRE(main.contains("\"Main.hpp\""_el));
        REQUIRE_FALSE(main.contains("Tls.hpp"_el));
        REQUIRE_FALSE(main.contains("Deep.hpp"_el));
        REQUIRE_FALSE(main.contains("Extra.hpp"_el));
        REQUIRE_FALSE(main.contains("Zip.hpp"_el));
        REQUIRE_FALSE(main.contains("/all.hpp"_el));
        const auto tls = read("src/sample/net/tls/all.hpp"_el);
        REQUIRE(tls.contains("\"Tls.hpp\""_el));
        REQUIRE_FALSE(tls.contains("Deep.hpp"_el));
        REQUIRE_FALSE(tls.contains("Extra.hpp"_el));
        REQUIRE(read("src/sample/net/tls/deep/all.hpp"_el).contains("\"Deep.hpp\""_el));
        REQUIRE(read("src/sample/net/zip/all.hpp"_el).contains("\"Zip.hpp\""_el));
        REQUIRE_FALSE(read("src/sample/empty/all.hpp"_el).contains("#include"_el));
        for (
            const auto &path : el::StringList{
                "include/sample/all_net_tls.hpp"_el,
                "include/sample/net/tls/all.hpp"_el,
                "include/sample/all_net_zip.hpp"_el,
                "include/sample/net/zip/all.hpp"_el,
                "include/sample/zip/all.hpp"_el,
                "include/sample/all_empty.hpp"_el,
                "include/sample/empty/all.hpp"_el}) {
            REQUIRE((_root / path).info().isRegularFile());
        }
        REQUIRE_FALSE((_root / "include/sample/all.hpp"_el).info().exists());
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testGlobalAndInheritedLocalExclusions() {
        configure(
            "[update_public_headers.directory.net]\nexcluded_from_all_headers: \"Generated.hpp\"\n"
            "[update_public_headers.directory.net.child]\n"
            "[update_public_headers.directory.net.child.deep]\nexcluded_from_all_headers: \"Local.hpp\"\n"_el,
            "excluded_from_all_headers: \"StdFormat.hpp\", \"Namespaces.hpp\"\n"_el);
        for (const auto &directory : el::StringList{"net"_el, "net/child"_el, "net/child/deep"_el}) {
            for (const auto &name : el::StringList{"StdFormat.hpp"_el, "Generated.hpp"_el, "Local.hpp"_el}) {
                write(el::join("src/sample/"_el, directory, "/"_el, name), "#pragma once\n"_el);
            }
        }
        REQUIRE_EQUAL(run(), 0);
        for (const auto &directory : el::StringList{"net"_el, "net/child"_el, "net/child/deep"_el}) {
            REQUIRE_FALSE(read(el::join("src/sample/"_el, directory, "/all.hpp"_el)).contains("StdFormat.hpp"_el));
            REQUIRE((_root / "include/sample"_el / el::Path{directory} / "StdFormat.hpp"_el).info().isRegularFile());
        }
        REQUIRE_FALSE(read("src/sample/net/child/all.hpp"_el).contains("Generated.hpp"_el));
        const auto deep = read("src/sample/net/child/deep/all.hpp"_el);
        REQUIRE(deep.contains("Generated.hpp"_el));
        REQUIRE_FALSE(deep.contains("Local.hpp"_el));
        REQUIRE_FALSE(read("src/sample/core/all.hpp"_el).contains("Namespaces.hpp"_el));
        REQUIRE((_root / "include/sample/Namespaces.hpp"_el).info().isRegularFile());
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testDestinationDefaultsAndLocalOverrides() {
        configure(
            "[update_public_headers.directory.net]\nflatten: Yes\ntarget_directory: \"custom\"\n"
            "[update_public_headers.directory.net.flat]\n"
            "[update_public_headers.directory.net.group]\nflatten: No\n"
            "[update_public_headers.directory.net.group.root]\ntarget_directory: \"\"\n"
            "[update_public_headers.directory.net.group.root.child]\n"
            "[update_public_headers.directory.net.zip]\nnested_namespace: Yes\nflatten: No\ntarget_directory: \"zip\"\n"
            "[update_public_headers.directory.net.zip.detail]\n"_el);
        write("src/sample/net/Main.hpp"_el, "namespace sample::net { class Main {}; }\n"_el);
        write("src/sample/net/flat/Flat.hpp"_el, "namespace sample::net { class Flat {}; }\n"_el);
        write("src/sample/net/group/Group.hpp"_el, "namespace sample::net { class Group {}; }\n"_el);
        write("src/sample/net/group/root/Root.hpp"_el, "namespace sample::net { class Root {}; }\n"_el);
        write("src/sample/net/group/root/child/Child.hpp"_el, "namespace sample::net { class Child {}; }\n"_el);
        write("src/sample/net/zip/Zip.hpp"_el, "namespace sample::net::zip { class Zip {}; }\n"_el);
        write("src/sample/net/zip/detail/Detail.hpp"_el, "namespace sample::net::zip { class Detail {}; }\n"_el);
        REQUIRE_EQUAL(run(), 0);
        const auto mapping = read("map.txt"_el);
        REQUIRE(mapping.contains("net/Main.hpp;custom/Main.hpp\n"_el));
        REQUIRE(mapping.contains("net/flat/Flat.hpp;Flat.hpp\n"_el));
        REQUIRE(mapping.contains("net/group/Group.hpp;net/group/Group.hpp\n"_el));
        REQUIRE(mapping.contains("net/group/root/Root.hpp;Root.hpp\n"_el));
        REQUIRE(mapping.contains("net/group/root/child/Child.hpp;net/group/root/child/Child.hpp\n"_el));
        REQUIRE(mapping.contains("net/zip/Zip.hpp;zip/Zip.hpp\n"_el));
        REQUIRE(mapping.contains("net/zip/detail/Detail.hpp;net/zip/detail/Detail.hpp\n"_el));
        REQUIRE_FALSE((_root / "include/sample/custom/Flat.hpp"_el).info().exists());
        REQUIRE_FALSE((_root / "include/sample/zip/Detail.hpp"_el).info().exists());
        REQUIRE(read("src/sample/core/impl/MakeOneNamespace.hpp"_el).contains("using namespace ::sample::net;"_el));
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
    }

    void testInvalidGlobalExclusions() {
        configure("[update_public_headers.directory.net]\n"_el, "excluded_from_all_headers: 42\n"_el);
        write("src/sample/net/Main.hpp"_el, "namespace sample::net { class Main {}; }\n"_el);
        REQUIRE_EQUAL(run(), 1);
        configure("[update_public_headers.directory.net]\n"_el, "excluded_from_all_headers: \"child/*.hpp\"\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_FALSE((_root / "map.txt"_el).info().exists());
    }

private:
    /// Prepare a small configured source tree.
    void configure(const el::String &sections, const el::String &global = {}) {
        write("src/sample/core/Definitions.hpp"_el, "#pragma once\n"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            el::join(
                "[rendering]\n[update_public_headers]\nsource_base_directory: \"src/sample\"\n"
                "include_base_directory: \"include/sample\"\nbase_namespace: \"sample\"\n"
                "mapping_file: \"map.txt\"\nexcluded_directories: \"impl\"\n"_el,
                global,
                "[update_public_headers.directory.core]\nflatten: Yes\n"_el,
                sections));
    }
    /// Run the header generator against the fixture.
    auto run(const el::StringList &options = {}) -> int {
        return runTool<tool::headers::PublicHeadersTool>({}, {}, "update-public-headers"_el, options);
    }
    /// Read one fixture file.
    auto read(const el::String &path) -> el::String { return files().readText(el::Path{path}); }
};
