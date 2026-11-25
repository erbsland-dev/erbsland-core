// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CMakeCleanup.hpp"

#include "CleanupText.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringHashSet.hpp>

namespace tool::cleanup {

using namespace text;
using namespace el::text::literals;

CMakeCleanup::CMakeCleanup(const CleanupPolicy &policy, const IncludeIndex &index) : _policy{policy}, _index{index} {
}

auto CMakeCleanup::command(const el::StringList &lines, const el::ItemIndex start) -> std::optional<Command> {
    static const auto whitespace = el::CharSet{U' ', U'\t', U'\n'};
    static const auto unsupported = el::CharSet{U'"', U'#', U'$', U';', U'[', U'\\'};
    auto content = el::StringEditor{};
    auto end = start;
    while (end.isWithin(lines.count())) {
        const auto &line = lines.getRef(end++);
        // Expressions, quoted arguments and comments need a real CMake parser; preserve such files.
        if (line.containsOneOf(unsupported)) {
            return {};
        }
        content.append(line).append(" "_el);
        if (line.contains(")"_el)) {
            break;
        }
    }
    static const auto pattern = el::RegEx::compile(R"/(^\s*([a-z_]+)\s*\(([^()]*)\)\s*$)/"_el);
    const auto match = pattern->match(el::String{content});
    if (!match) {
        return {};
    }
    return Command{match->content(1), el::StringList::fromSplit(match->content(2), whitespace), end};
}

auto CMakeCleanup::render(const el::String &target, const el::StringList &files, const el::String &indent)
    -> el::String {
    auto names = el::StringHashSet{};
    auto unique = el::StringList{};
    for (const auto &name : files) {
        if (names.tryInsert(name)) {
            unique.append(name);
        }
    }
    auto output = el::StringEditor{};
    output.append(indent).append("target_sources("_el).append(target).append(" PRIVATE\n"_el);
    for (const auto &name : unique.sorted(&el::Char::compareCaseFolded)) {
        output.append(indent).append("        "_el).append(name).append("\n"_el);
    }
    return output.append(indent).append(")\n"_el);
}

auto CMakeCleanup::validFile(const el::Path &directory, const el::String &name) const -> bool {
    // These are user-written CMake arguments, not walker results. Only local filenames are supported.
    return !name.isEmpty() && !name.containsOneOf(el::CharSet{U'/', U'\\'}) && _index.contains(directory / name);
}

auto CMakeCleanup::sources(Document &document, const Command &command, const bool conditional) const -> bool {
    const auto &arguments = command.arguments;
    if (arguments.count() < el::ItemCount{2} || arguments.getRef(el::ItemIndex{1}) != "PRIVATE"_el ||
        arguments.first() != document.target) {
        return false;
    }
    for (auto index = el::ItemIndex{2}; index.isWithin(arguments.count()); ++index) {
        const auto &name = arguments.getRef(index);
        if (name == "PUBLIC"_el || name == "INTERFACE"_el || name == "PRIVATE"_el || name == "FILE_SET"_el) {
            return false;
        }
        if (document.invalidFile.isEmpty() && !validFile(document.directory, name)) {
            document.invalidFile = name;
        }
        document.covered.insert(name);
        if (!conditional) {
            document.mainFiles.append(name);
        }
    }
    return true;
}

auto CMakeCleanup::accept(Document &document, const Command &command, const bool conditional) const -> bool {
    if (command.name == "target_sources"_el) {
        return sources(document, command, conditional);
    }
    if (conditional) {
        return false;
    }
    if (command.name == "cmake_minimum_required"_el) {
        // The managed minimum replaces only the simple VERSION form, never extra policies or ranges.
        static const auto version = el::RegEx::compile(R"/([0-9]+(?:\.[0-9]+){1,3})/"_el);
        return command.arguments.count() == el::ItemCount{2} && command.arguments.first() == "VERSION"_el &&
            version->fullMatch(command.arguments.last());
    }
    if (command.name != "add_subdirectory"_el || command.arguments.count() != el::ItemCount{1}) {
        return false;
    }
    static const auto directoryName = el::RegEx::compile(R"/([A-Za-z0-9_\-]+)/"_el);
    const auto &name = command.arguments.first();
    if (!directoryName->fullMatch(name)) {
        return false;
    }
    if (!(document.directory / name).info().isDirectory()) {
        document.invalidDirectory = name;
    }
    if (!document.directories.contains(name)) {
        document.directories.append(name);
    }
    return true;
}

auto CMakeCleanup::parse(Document &document, const el::StringList &lines) const -> bool {
    static const auto conditionalStart = el::RegEx::compile(R"/(if\s*\(.*)/"_el);
    static const auto controlFlow = el::RegEx::compile(R"/((?:if|elseif|else|endif)\s*\(.*)/"_el);
    auto conditional = false;
    auto line = el::ItemIndex{0};
    while (line.isWithin(lines.count())) {
        const auto &original = lines.getRef(line);
        const auto value = original.trimmed();
        conditional = conditional || conditionalStart->fullMatch(value);
        if (conditional && (value.isEmpty() || value.startsWith("#"_el) || controlFlow->fullMatch(value))) {
            document.suffix.append(original);
            ++line;
            continue;
        }
        if (!conditional &&
            (value.isEmpty() || value.startsWith("# Copyright"_el) ||
                value.startsWith("# SPDX-License-Identifier:"_el))) {
            ++line;
            continue;
        }
        const auto parsed = command(lines, line);
        if (!parsed || !accept(document, *parsed, conditional)) {
            return false;
        }
        if (conditional) {
            const auto indent = original.slice(el::StringSide::Front, original.find("target_sources"_el));
            const auto names = parsed->arguments.suffix(parsed->arguments.count() - el::ItemCount{2});
            document.suffix.append(text::lines(render(document.target, names, indent)));
        }
        line = parsed->end;
    }
    return true;
}

auto CMakeCleanup::renderDocument(Document document, const el::StringList &localFiles, const el::String &header) const
    -> el::String {
    for (const auto &name : localFiles) {
        if (!document.covered.contains(name)) {
            document.mainFiles.append(name);
        }
    }
    auto output = el::StringEditor{header};
    output.append("\n\ncmake_minimum_required(VERSION "_el).append(_policy.cmakeVersion).append(")\n"_el);
    if (!document.directories.isEmpty()) {
        output.append("\n"_el);
        for (const auto &directory : document.directories.sort(&el::Char::compareCaseFolded)) {
            output.append("add_subdirectory("_el).append(directory).append(")\n"_el);
        }
    }
    if (!document.mainFiles.isEmpty()) {
        output.append("\n"_el).append(render(document.target, document.mainFiles));
    }
    if (!document.suffix.isEmpty()) {
        output.append("\n"_el).append(join(document.suffix));
    }
    return output;
}

auto CMakeCleanup::apply(
    const el::Path &path, const el::String &source, const el::StringList &localFiles, const el::String &header) const
    -> std::optional<el::String> {
    auto document = Document{path.parent(), _policy.cmakeTarget(path.parent()), {}, {}, {}, {}, {}, {}};
    if (!parse(document, lines(source))) {
        return {};
    }
    if (!document.invalidFile.isEmpty()) {
        fail("CMake source entry must name an existing directory-local file."_el, document.invalidFile);
    }
    if (!document.invalidDirectory.isEmpty()) {
        fail("CMake subdirectory does not exist."_el, document.invalidDirectory);
    }
    return renderDocument(std::move(document), localFiles, header);
}

}
