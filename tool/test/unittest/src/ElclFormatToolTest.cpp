// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/conf/syntax/Formatter.hpp>
#include <erbsland/conf/syntax/Parser.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StringEncoder.hpp>
#include <tool/elcl_format/ElclFormatTool.hpp>
#include <tool/elcl_format/FormatConfig.hpp>

#include <filesystem>

using namespace el::text::literals;
TESTED_TARGETS(ElclFormatTool FormatConfig)
class ElclFormatToolTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
private:
    auto run(std::initializer_list<el::String> options) -> int {
        auto output = el::stream::AnyStringBuilderStream::create();
        auto error = el::stream::AnyStringBuilderStream::create();
        const auto redirect = el::stream::redirectStandardStreams(output, error);
        auto storage = std::vector<std::string>{
            el::StringConverter{"erbsland-core-tool"_el}.toStdString(),
            el::StringConverter{"elcl-format"_el}.toStdString()};
        for (const auto &option : options) {
            storage.push_back(el::StringConverter{option}.toStdString());
        }
        auto arguments = std::vector<char *>{};
        for (auto &arg : storage) {
            arguments.push_back(arg.data());
        }
        const auto count = static_cast<int>(arguments.size());
        arguments.push_back(nullptr);
        auto application = TestToolApplication{count, arguments.data()};
        application.registerTool<tool::elcl_format::ElclFormatTool>();
        const auto result = application.run();
        application.log().shutdown();
        _outputText = output->toString();
        _errorText = error->toString();
        return result;
    }
    auto external() -> el::Path {
        const auto path = _temporary->path() / "external"_el;
        path.operations().createDirectoryOrThrow();
        return path;
    }
    void writeFile(const el::Path &path, const el::String &text) { path.content().writeTextOrThrow(text); }

public:
    void testConfiguredIndentationAndInvalidValues() {
        const auto configuration = external() / "format.elcl"_el;
        writeFile(configuration, "[elcl_format.base.indentation]\nstyle: \"tabs\"\ncount: 2\n"_el);
        const auto format = tool::elcl_format::FormatConfig{configuration}.read();
        REQUIRE(format.baseStyle().indentationStyle() == el::conf::syntax::IndentationStyle::Tabs);
        REQUIRE_EQUAL(format.baseStyle().indentationCount(), el::CpCount{2});
        for (
            const auto source :
            {"[elcl_format.base.indentation]\nstyle: \"unknown\"\n"_el,
                "[elcl_format.base.indentation]\ncount: 0\n"_el}) {
            configuration.content().replaceTextOrThrow(source);
            REQUIRE_THROWS(tool::elcl_format::FormatConfig{configuration}.read());
        }
    }

    void testConfiguredValueWrappingPoliciesAndOverrides() {
        const auto configuration = external() / "format.elcl"_el;
        writeFile(
            configuration,
            "[elcl_format.base]\nvalue_wrapping: \"next_line\"\n*[elcl_format.rules]\n[.selector]\npaths: \"a.keep\"\n[.style]\nvalue_wrapping: \"same_line\"\n"_el);
        const auto format = tool::elcl_format::FormatConfig{configuration}.read();
        const auto document = el::conf::syntax::Parser{}.parseText("[a]\nx:1\nkeep:2\n"_el);
        const auto text = el::conf::syntax::Formatter{format}.format(document).text();
        REQUIRE(text.contains("X                            :\n    1"_el));
        REQUIRE(text.contains("Keep                         : 2"_el));
        configuration.content().replaceTextOrThrow("[elcl_format.base]\nvalue_wrapping: \"unknown\"\n"_el);
        REQUIRE_THROWS(tool::elcl_format::FormatConfig{configuration}.read());
    }

    void testExternalOutputOnlyAndUnchangedTimestamp() {
        const auto directory = external();
        const auto file = directory / "settings.elcl"_el;
        writeFile(file, "[app]\nvalue:1\n"_el);
        const auto original = file.content().readDataOrThrow();
        REQUIRE_EQUAL(run({"--output-only"_el, file.toString()}), 0);
        REQUIRE(_outputText.contains("Value"_el));
        REQUIRE(_errorText.isEmpty());
        REQUIRE_EQUAL(file.content().readDataOrThrow(), original);
        REQUIRE_EQUAL(run({file.toString()}), 0);
        auto info = file.info();
        info.reload();
        const auto timestamp = info.lastModified();
        const auto contents = file.content().readDataOrThrow();
        REQUIRE_EQUAL(run({file.toString()}), 0);
        info.reload();
        REQUIRE_EQUAL(info.lastModified(), timestamp);
        REQUIRE_EQUAL(file.content().readDataOrThrow(), contents);
    }
    void testAncestorConfigurationAndExplicitOverride() {
        const auto directory = external();
        const auto child = directory / "child"_el;
        child.operations().createDirectoryOrThrow();
        const auto file = child / "settings.elcl"_el;
        writeFile(file, "[app]\nvalue:1\n"_el);
        const auto config = directory / "elcl_format.elcl"_el;
        writeFile(config, "[Elcl Format.Base.Names]\nValue Case: \"upper\"\n"_el);
        REQUIRE_EQUAL(run({"--output-only"_el, file.toString()}), 0);
        REQUIRE(_outputText.contains("VALUE"_el));
        writeFile(child / "elcl_format.elcl"_el, "[Elcl Format.Base.Names]\nValue Case: \"lower\"\n"_el);
        REQUIRE_EQUAL(run({"--output-only"_el, file.toString()}), 0);
        REQUIRE(_outputText.contains("value"_el));
        REQUIRE_EQUAL(run({"--output-only"_el, "--config"_el, config.toString(), file.toString()}), 0);
        REQUIRE(_outputText.contains("VALUE"_el));
    }
    void testRecursiveSelectionAndOutputCardinality() {
        const auto directory = external();
        const auto child = directory / "child"_el;
        child.operations().createDirectoryOrThrow();
        writeFile(directory / "a.elcl"_el, "[a]\nx:1\n"_el);
        writeFile(child / "b.elcl"_el, "[b]\ny:2\n"_el);
        writeFile(directory / "skip.txt"_el, "[c]\nz:3\n"_el);
        REQUIRE_EQUAL(run({"--output-only"_el, directory.toString()}), 0);
        REQUIRE_EQUAL(run({"--output-only"_el, "--recursive"_el, directory.toString()}), 1);
        REQUIRE(_outputText.isEmpty());
        REQUIRE_EQUAL(run({"--recursive"_el, directory.toString()}), 0);
        REQUIRE((child / "b.elcl"_el).content().readTextOrThrow().contains("Y"_el));
        REQUIRE_EQUAL((directory / "skip.txt"_el).content().readTextOrThrow(), "[c]\nz:3\n"_el);
    }
    void testConfigurationFailurePreventsWrites() {
        const auto directory = external();
        const auto file = directory / "settings.elcl"_el;
        const auto source = "[a]\nx:1\n"_el;
        writeFile(file, source);
        writeFile(directory / "elcl_format.elcl"_el, "[Elcl Format.Base]\nUnknown: 1\n"_el);
        REQUIRE_EQUAL(run({file.toString()}), 1);
        REQUIRE_EQUAL(file.content().readTextOrThrow(), source);
    }
    void testSourceErrorsAndRuleCascade() {
        const auto directory = external();
        const auto file = directory / "settings.elcl"_el;
        writeFile(file, "[a]\nbad: ???\ngood:1\n"_el);
        writeFile(
            directory / "elcl_format.elcl"_el,
            "*[Elcl Format.Rules]\n[.Selector]\nPaths: \"a.good\"\n[.Style.Names]\nValue Case: \"upper\"\n"_el);
        REQUIRE_EQUAL(run({file.toString()}), 1);
        const auto formatted = file.content().readTextOrThrow();
        REQUIRE(formatted.contains("bad: ???\n"_el));
        REQUIRE(formatted.contains("GOOD"_el));
    }
    void testDuplicatesAndProjectFlag() {
        const auto directory = external();
        const auto file = directory / "settings.elcl"_el;
        writeFile(file, "[a]\nx:1\n"_el);
        REQUIRE_EQUAL(run({"--output-only"_el, file.toString(), directory.toString()}), 0);
        REQUIRE_EQUAL(run({"--project"_el, _root.toString(), file.toString()}), 1);
        REQUIRE_EQUAL(run({"--project"_el, ""_el, file.toString()}), 1);
    }
    void testInvalidInputPreventsPartialWrites() {
        const auto directory = external();
        const auto file = directory / "settings.elcl"_el;
        const auto source = "[a]\nx:1\n"_el;
        writeFile(file, source);
        REQUIRE_EQUAL(run({file.toString(), (directory / "missing.elcl"_el).toString()}), 1);
        REQUIRE(_errorText.contains("Invalid ELCL Input"_el));
        REQUIRE_EQUAL(file.content().readTextOrThrow(), source);
    }
    void testBomPermissionsAndInvalidEncoding() {
        const auto directory = external();
        const auto file = directory / "settings.elcl"_el;
        const auto original =
            el::StringEncoder{el::String{"[a]\r\nx:1"_el}}.encode(el::StringEncoding::Utf8, el::StringBomMode::Require);
        file.content().writeDataOrThrow(original);
        const auto permissions = std::filesystem::status(file.toStdPath()).permissions();
        REQUIRE_EQUAL(run({file.toString()}), 0);
        const auto expectedText =
            el::conf::syntax::Formatter{}.format(el::conf::syntax::Parser{}.parseText("[a]\r\nx:1"_el)).text();
        const auto expected =
            el::StringEncoder{expectedText}.encode(el::StringEncoding::Utf8, el::StringBomMode::Require);
        REQUIRE_EQUAL(file.content().readDataOrThrow(), expected);
        REQUIRE(std::filesystem::status(file.toStdPath()).permissions() == permissions);
        const auto invalid = el::ByteBlock{0xffU, 0xfeU, 0xffU};
        file.content().replaceDataOrThrow(invalid);
        REQUIRE_EQUAL(run({file.toString()}), 1);
        REQUIRE_EQUAL(file.content().readDataOrThrow(), invalid);
    }
    void testSymlinkInputsAndTraversal() {
        const auto directory = external();
        const auto file = directory / "settings.elcl"_el;
        writeFile(file, "[a]\nx:1\n"_el);
        const auto link = directory / "link.elcl"_el;
        auto error = std::error_code{};
        std::filesystem::create_symlink(file.toStdPath(), link.toStdPath(), error);
        if (error) {
            return;
        } // Some Windows hosts do not grant symlink privileges.
        REQUIRE_EQUAL(run({"--output-only"_el, link.toString()}), 1);
        REQUIRE_EQUAL(run({"--output-only"_el, directory.toString()}), 0);
        REQUIRE(std::filesystem::is_symlink(link.toStdPath()));
    }
};
