// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DemoCreation.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/Capitalization.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/render/Value.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringTransformations.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

DemoCreation::DemoCreation(
    const frame::ProjectFiles &files, el::render::EnvironmentPtr renderer, const el::String &path) :
    _files{files}, _renderer{std::move(renderer)} {
    parsePath(path);
    const auto domain = _files.projectRoot() / "demos"_el / _domain;
    const auto demo = _files.checked(domain / _name, true);
    const auto source = el::join(_part, ".cpp"_el);
    if (_files.checked(demo / source, true).info().exists()) {
        throwError("Demo part already exists."_el, (demo / source).toString());
    }
    auto parts = Parts{};
    parts.sources = el::StringList{el::join(_name, "Demos.hpp"_el), source, "main.cpp"_el};
    parts.functions.append(_function);
    parts.registrations.set(_part, _function);
    if (demo.info().exists()) {
        readExistingParts(demo, parts);
    }
    prepareFiles(demo, parts);
    directory(domain / "CMakeLists.txt"_el, _name);
    directory(_files.projectRoot() / "demos/CMakeLists.txt"_el, _domain);
    warnMissingHeader();
}

void DemoCreation::parsePath(const el::String &path) {
    static const auto reDemoLocation = el::RegEx::compile(R"re((?x)
        ^ (?<domain>[_a-z]+) /             # Domain directory
        (?<name>[A-Z][a-zA-Z0-9]*) /       # Demo directory
        (?<part>[A-Z][a-zA-Z0-9]*) $       # Demo part name
    )re"_el);

    const auto parsed = reDemoLocation->match(path);
    if (!parsed) {
        throw el::ParseError{"Expected <domain>/<DemoName>/<DemoPart>."_el};
    }
    _domain = parsed->content("domain"_el);
    _name = parsed->content("name"_el);
    _part = parsed->content("part"_el);
    auto function = el::StringEditor{};
    function.append(_part.charAt(el::StringSide::Front).toLowercase());
    function.append(_part.slice(el::CpRange::fromBegin(el::CpIndex{1})));
    _function = function;
    _target = el::toAsciiWordsFromCamelCase(_name, U'_', el::Capitalization::Lowercase);
}

void DemoCreation::readExistingParts(const el::Path &demo, Parts &parts) {
    static const auto reDemoTarget = el::RegEx::compile(R"re((?x)
        erbsland_core_add_demo \( \s*      # Demo target registration
        (?<target>[-_A-Za-z0-9]+)          # Target identifier
        \s* \)                             # End of registration
    )re"_el);
    static const auto reDemoSources = el::RegEx::compile(R"re((?x)
        (?s) target_sources \( \s*         # Source-list declaration
        [-_A-Za-z0-9]+ \s+ PRIVATE \s*     # Target and visibility
        (?<sources>.*?) \)                 # Source entries through the closing parenthesis
    )re"_el);
    static const auto reCmakeComment = el::RegEx::compile(R"re(#[^\n]*)re"_el);
    static const auto reDemoDeclaration = el::RegEx::compile(R"re((?x)
        (?m) ^ \s* void \s+                # Demo function declaration
        (?<name>[A-Za-z][A-Za-z0-9]*)      # Function identifier
        \(\); \s* $                        # Empty parameter list and declaration terminator
    )re"_el);
    static const auto reDemoRegistration = el::RegEx::compile(R"re((?x)
        app\.registerDemo \( "             # Registration call and string literal opening
        (?<name>[A-Za-z][A-Za-z0-9]*)      # Public demo name
        "_el, \s*                          # Literal suffix and argument separator
        (?<function>[A-Za-z][A-Za-z0-9]*)  # Function identifier
        \);                                # End of registration statement
    )re"_el);

    static const auto cCmakeWhitespace = el::CharSet{U' ', U'\t', U'\n', U'\r'};
    const auto header = el::join(_name, "Demos.hpp"_el);
    const auto cmake = _files.readText(demo / "CMakeLists.txt"_el);
    const auto main = _files.readText(demo / "main.cpp"_el);
    const auto declarations = _files.readText(demo / header);
    if (!main.contains("DemoApplication"_el) || !main.contains("return app.run();"_el)) {
        throwError("Existing demo does not use the multi-part schema."_el, (demo / "main.cpp"_el).toString());
    }
    const auto target = reDemoTarget->findFirst(cmake);
    const auto existing = reDemoSources->findFirst(cmake);
    if (!target || !existing) {
        throwError("Existing demo CMake schema is unsupported."_el, (demo / "CMakeLists.txt"_el).toString());
    }
    _target = target->content("target"_el);
    for (
        const auto &entry : el::StringList::fromSplit(
            reCmakeComment->replaceAll(existing->content("sources"_el), ""_el), cCmakeWhitespace)) {
        if (!parts.sources.contains(entry)) {
            parts.sources.append(entry);
        }
    }
    for (const auto &declaration : reDemoDeclaration->findAll(declarations)) {
        const auto name = declaration->content("name"_el);
        if (!parts.functions.contains(name)) {
            parts.functions.append(name);
        }
    }
    for (const auto &registration : reDemoRegistration->findAll(main)) {
        parts.registrations.set(registration->content("name"_el), registration->content(cFunction));
    }
}

void DemoCreation::prepareFiles(const el::Path &demo, Parts &parts) {
    const auto header = el::join(_name, "Demos.hpp"_el);
    const auto source = el::join(_part, ".cpp"_el);
    parts.sources.sort();
    parts.functions.sort();
    auto context = el::render::Context{};
    context.set("sources"_el, parts.sources);
    add(demo / "CMakeLists.txt"_el, render("doc/demo_cmake.cpp"_el, context));
    context = {};
    context.set("functions"_el, parts.functions);
    add(demo / header, render("doc/demo_header.cpp"_el, context));
    auto registrationValues = el::render::ValueList{};
    for (const auto &[name, functionName] : parts.registrations) {
        auto value = el::render::ValueMap{};
        value.set("name"_el, name);
        value.set(cFunction, functionName);
        registrationValues.append(value);
    }
    context = {};
    context.set("registrations"_el, registrationValues);
    add(demo / "main.cpp"_el, render("doc/demo_main.cpp"_el, context));
    add(demo / source, render("doc/demo_source.cpp"_el));
}

void DemoCreation::warnMissingHeader() {
    const auto library = _files.checked(_files.projectRoot() / "src/erbsland"_el / _domain, true);
    if (!library.info().isDirectory()) {
        _warnings.append(el::join("Library domain does not exist: "_el, _domain));
    } else {
        auto found = false;
        for (const auto &path : _files.collectFilenamesOnly(library, ".hpp"_el)) {
            if (path.name() == el::join(_name, ".hpp"_el)) {
                found = true;
                break;
            }
        }
        if (!found) {
            _warnings.append(el::join("No matching library header: "_el, _name, ".hpp"_el));
        }
    }
}

auto DemoCreation::render(const el::String &layout, el::render::Context context) const -> el::String {
    context.set(cFunction, _function);
    context.set("target"_el, _target);
    context.set("header"_el, el::join(_name, "Demos.hpp"_el));
    return _renderer->render(layout, context);
}

void DemoCreation::add(const el::Path &path, const el::String &text) {
    const auto checked = _files.checked(path, true);
    if (checked.info().exists() && !checked.info().isRegularFile()) {
        throwError("Demo destination is not a regular file."_el, checked.toString());
    }
    if (!checked.info().exists() || _files.readText(checked) != text) {
        _updates.set(checked.toString(), text);
    }
}

void DemoCreation::directory(const el::Path &path, const el::String &entry) {
    static const auto reSubdirectory = el::RegEx::compile(R"re((?x)
        (?m) ^ add_subdirectory \( \s*    # CMake child registration
        (?<name>[-_a-zA-Z0-9.]+)          # Child directory name
        \s* \) \s* $                      # Closing parenthesis and line boundary
    )re"_el);

    const auto checked = _files.checked(path, true);
    auto entries = el::StringList{entry};
    if (checked.info().isRegularFile()) {
        const auto foundExpression = reSubdirectory;
        for (const auto &found : foundExpression->findAll(_files.readText(checked))) {
            const auto name = found->content("name"_el);
            if (!entries.contains(name)) {
                entries.append(name);
            }
        }
    }
    entries.sort();
    auto context = el::render::Context{};
    context.set("entries"_el, entries);
    add(path, render("doc/directory_cmake.cpp"_el, context));
}

void DemoCreation::apply() const {
    for (const auto &[name, text] : _updates) {
        _files.createDirectories(el::Path{name}.parent());
        _files.updateText(el::Path{name}, text);
    }
}

auto DemoCreation::documentation() const -> el::String {
    return el::join(
        ".. erbsland-demo::\n    :source: "_el,
        _domain,
        "/"_el,
        _name,
        "/"_el,
        _part,
        ".cpp\n    :exec: "_el,
        _domain,
        "/"_el,
        _target,
        " --demo "_el,
        _part,
        "\n\n.. erbsland-demo-end::\n"_el);
}

void DemoCreation::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"Demo Creation Failed"_el, description}.withSourcePath(path);
}

}
