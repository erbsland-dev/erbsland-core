// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DisplayTextCompiler.hpp"

#include <erbsland/CharSet.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/EscapeAmount.hpp>
#include <erbsland/EscapeFormat.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathCreateMode.hpp>
#include <erbsland/PathReadTextOptions.hpp>
#include <erbsland/PathWriteTextOptions.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/render/ResourceLoader.hpp>
#include <erbsland/StringBomMode.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>

#include <utility>

namespace erbsland::i18n::compiler {

using namespace err;
using namespace path;
using namespace text;
using namespace text::literals;
using text::String;

DisplayTextCompiler::DisplayTextCompiler(String className, String namespaceName) :
    _className{std::move(className)}, _namespaceName{std::move(namespaceName)} {
    if (!validIdentifier(_className)) {
        throw LogicFailure{"The display-text class must be a non-reserved C++ identifier."_el};
    }
    if (!_namespaceName.isEmpty()) {
        static const auto namespacePattern = re::RegEx::compile("^[A-Za-z][A-Za-z0-9_]*(::[A-Za-z][A-Za-z0-9_]*)*$"_el);
        if (!namespacePattern->fullMatch(_namespaceName)) {
            throw LogicFailure{"Invalid display-text namespace."_el};
        }
        const auto parts = StringList::fromSplit(_namespaceName, CharSet{U':'});
        for (const auto &part : parts) {
            if (!validIdentifier(part)) {
                throw LogicFailure{"The display-text namespace must contain non-reserved C++ identifiers."_el};
            }
        }
    }
}

auto DisplayTextCompiler::validIdentifier(const String &name) -> bool {
    static const auto identifier = re::RegEx::compile("^[A-Za-z][A-Za-z0-9_]*$"_el);
    static const auto keywords = String{
        " alignas alignof and and_eq asm atomic_cancel atomic_commit atomic_noexcept auto bitand bitor bool break "
        "case catch char char8_t char16_t char32_t class compl concept const consteval constexpr constinit const_cast "
        "continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum explicit export "
        "extern false float for friend goto if inline int long mutable namespace new noexcept not not_eq nullptr "
        "operator or or_eq private protected public reflexpr register reinterpret_cast requires return short signed "
        "sizeof static static_assert static_cast struct switch synchronized template this thread_local throw true try "
        "typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq "_el};
    return identifier->fullMatch(name) != nullptr && !name.contains("__"_el) &&
        !keywords.contains(el::join(" "_el, name, " "_el));
}

void DisplayTextCompiler::compile(const Path &input, const Path &header, const Path &source) {
    if (input == header || input == source || header == source) {
        throw LogicFailure{"Catalog input, header, and source paths must be distinct."_el};
    }
    auto parser = conf::Parser{};
    parser.setSourceResolver(nullptr);
    const auto document = parser.parseFileOrThrow(input);
    const auto headerInclude =
        header.toAbsoluteOrThrow().toRelativeOrThrow(source.toAbsoluteOrThrow().parent()).toString();
    const auto output = generate(document, headerInclude);
    writeIfChanged(header, output.header);
    writeIfChanged(source, output.source);
}

auto DisplayTextCompiler::generate(
    const conf::ValuePtr &document, const String &headerInclude, render::EnvironmentPtr renderer) -> DisplayTextOutput {
    _domains.clear();
    for (const auto &section : *document) {
        if (!section->name().isRegular() || !section->isSectionWithNames()) {
            throw conf::ConfError{conf::ConfErrorContext{
                conf::ConfErrorCategory::TypeMismatch,
                "Display-text catalogs require named sections."_el,
                section->location()}};
        }
        auto domain = section->name().asText();
        if (domain == "root"_el) {
            domain = {};
        }
        collect(section, domain);
    }
    if (!renderer) {
        auto options = render::EnvironmentOptions{};
        options.setExpressionDelimiters(render::Delimiters{"/*{"_el, "}*/"_el});
        options.setStatementDelimiters(render::Delimiters{"/*%"_el, "%*/"_el});
        options.setAutomaticEscapingEnabled(false);
        renderer = render::Environment::create(options);
        auto globals = render::Context{};
        globals.set("copyright_line"_el, String{});
        globals.set("license_line"_el, String{});
        renderer->setGlobalContext(std::move(globals));
    }
    renderer->addLayoutLoader(render::ResourceLoader::create("erbsland-display-text-compiler"_el));
    auto domains = render::ValueList{};
    auto index = std::size_t{};
    for (const auto &[name, values] : _domains) {
        auto entries = render::ValueList{};
        for (const auto &[key, value] : values) {
            entries.append(
                render::ValueMap{
                    {"key"_el, key.toEscaped(EscapeFormat::Cpp, EscapeAmount::NonAscii)},
                    {"text"_el, value.toEscaped(EscapeFormat::Cpp, EscapeAmount::NonAscii)}});
        }
        domains.append(
            render::ValueMap{
                {"name"_el, name.toEscaped(EscapeFormat::Cpp, EscapeAmount::NonAscii)},
                {"index"_el, index++},
                {"count"_el, values.count().toSizeT()},
                {"entries"_el, std::move(entries)}});
    }
    auto context = render::Context{};
    context.set("class_name"_el, _className);
    context.set("namespace_name"_el, _namespaceName);
    context.set("header_include"_el, headerInclude);
    context.set("domain_count"_el, _domains.count().toSizeT());
    context.set("domains"_el, std::move(domains));
    return {renderer->render("display_text.hpp"_el, context), renderer->render("display_text.cpp"_el, context)};
}

void DisplayTextCompiler::collect(const conf::ValuePtr &section, const String &domain) {
    auto entries = _domains.get(domain, Entries{});
    for (const auto &value : *section) {
        if (!value->name().isRegular()) {
            throw conf::ConfError{conf::ConfErrorContext{
                conf::ConfErrorCategory::TypeMismatch,
                "Display-text names must be regular ELCL names."_el,
                value->location()}};
        }
        const auto name = value->name().asText();
        if (value->isSectionWithNames()) {
            auto childDomain = name;
            if (!domain.isEmpty()) {
                childDomain = el::join(domain, "."_el, name);
            }
            collect(value, childDomain);
        } else if (value->isText()) {
            if (!entries.tryInsert(name, value->asText())) {
                throw conf::ConfError{conf::ConfErrorContext{
                    conf::ConfErrorCategory::Syntax, "Duplicate normalized display-text name."_el, value->location()}};
            }
        } else {
            throw conf::ConfError{conf::ConfErrorContext{
                conf::ConfErrorCategory::TypeMismatch,
                "Display-text entries must be text values or named sections."_el,
                value->location()}};
        }
    }
    _domains.set(domain, std::move(entries));
}

void DisplayTextCompiler::writeIfChanged(const Path &path, const String &content) {
    auto readOptions = PathReadTextOptions{};
    readOptions.setMaximumByteLength(unit::ByteCount::infinite());
    if (const auto existing = path.content().readText(readOptions); existing.has_value() && *existing == content) {
        return;
    }
    auto options = PathWriteTextOptions{StringEncoding::Utf8};
    options.setBomMode(StringBomMode::Reject).setCreationMode(PathCreateMode::CreateOrOverwrite).setCreateParents(true);
    path.content().writeTextOrThrow(content, options);
}

}
