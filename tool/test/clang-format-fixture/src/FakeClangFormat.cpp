// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/Path.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <string_view>

using namespace el::text::literals;

/// External formatter fixture exercising real subprocess capture and failure handling.
class FakeClangFormat final {
public:
    /// Execute the formatter fixture using native command-line arguments.
    /// @param argc Native argument count.
    /// @param argv Native argument strings.
    /// @return Zero on successful formatting, or a simulated failure.
    static auto run(const int argc, char *argv[]) -> int {
        if (argc == 2 && el::String{std::string_view{argv[1]}} == "--version"_el) {
            el::stream::stdOut()->write("fake-clang-format 1\n"_el);
            return 0;
        }
        if (argc != 4 || el::String{std::string_view{argv[2]}} != "--fail-on-incomplete-format"_el) {
            return 90;
        }
        const auto source = el::Path{el::String{std::string_view{argv[3]}}};
        const auto text = source.content().readTextOrThrow();
        if (text.contains("FORMAT_FAILURE"_el)) {
            el::stream::stdErr()->write("simulated formatter failure\n"_el);
            return 2;
        }
        if (text.contains("FORMAT_EMPTY_OUTPUT"_el)) {
            return 0;
        }
        if (text.contains("FORMAT_BOM"_el)) {
            el::stream::stdOut()->write("\uFEFF"_el);
        }
        el::stream::stdOut()->write(text.replacedAll("int value=1;"_el, "int value = 1;"_el));
        return 0;
    }
};

auto main(const int argc, char *argv[]) -> int {
    const auto result = FakeClangFormat::run(argc, argv);
    el::stream::stdOut()->flush();
    el::stream::stdErr()->flush();
    return result;
}
