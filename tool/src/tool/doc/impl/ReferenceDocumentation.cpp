// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReferenceDocumentation.hpp"

#include "HeaderScanner.hpp"

#include "../../cleanup/CleanupText.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/Capitalization.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathGlobPattern.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringPattern.hpp>
#include <erbsland/StringSet.hpp>
#include <erbsland/StringTransformations.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

auto ReferenceDocumentation::matches(const el::conf::ValuePtr &patterns, const el::String &name) -> bool {
    if (!patterns) {
        return false;
    }
    for (const auto &value : patterns->toValueList()) {
        try {
            if (el::StringPattern::fromGlob(value->asTextOrThrow()).matches(name)) {
                return true;
            }
        } catch (const el::ParseError &) {
            throw el::conf::ConfError{
                value,
                "Invalid Reference Exclusion Pattern"_el,
                "Expected a valid header glob."_el,
                std::current_exception()};
        }
    }
    return false;
}

auto ReferenceDocumentation::excluded(const el::Path &path) const -> bool {
    const auto directories = _configuration->getStringList("excluded_directory_names"_el);
    for (const auto &element : path.parent().elements()) {
        if (directories.contains(element)) {
            return true;
        }
    }
    if (_configuration->getStringList("excluded_header_names"_el).contains(path.name())) {
        return true;
    }
    if (_configuration->getBoolean("exclude_underscore_headers"_el, true) && path.name().contains("_"_el)) {
        return true;
    }
    return matches(_configuration->value(cExcludedHeaderGlobs), path.toString());
}

ReferenceDocumentation::ReferenceDocumentation(
    const frame::ProjectFiles &files, const el::conf::ValuePtr &configuration) :
    _files{files},
    _configuration{configuration},
    _source{files.checked(configuration->valueOrThrow("source_directory"_el), el::PathType::Directory)},
    _reference{files.checked(configuration->valueOrThrow("reference_directory"_el), true)},
    _manualPages{configuration->getStringList(cManualReferencePages)} {
    auto claimed = el::StringSet{};
    auto pages = el::StringSet{};
    auto available = el::StringList{};
    for (const auto &path : files.collectFilenamesOnly(_source, ".hpp"_el)) {
        const auto relative = path.toRelative(_source);
        if (!excluded(relative)) {
            available.append(relative.toString());
        }
    }
    available.sort([](const el::String &left, const el::String &right) {
        return el::toCaseFolded(left) < el::toCaseFolded(right);
    });
    if (const auto groups = configuration->getSectionList("reference_groups"_el)) {
        for (const auto &entry : *groups) {
            const auto pageText = entry->getTextOrThrow(cPage);
            const auto page = el::Path{pageText};
            const auto absolute = referencePath(entry->valueOrThrow(cPage));
            if (page.suffix() != ".rst"_el || page.name() == "index.rst"_el || pages.contains(absolute.toString())) {
                throw el::conf::ConfError{
                    entry->valueOrThrow(cPage),
                    "Invalid Reference Group Page"_el,
                    "Each reference group requires a unique RST page below the reference directory, excluding index.rst."_el};
            }
            pages.insert(absolute.toString());
            auto headers = entry->getStringList(cHeaders);
            for (const auto &header : headers) {
                const auto value = entry->valueOrThrow(cHeaders);
                if (!available.contains(header)) {
                    throw el::conf::ConfError{
                        value, "Invalid Reference Group Header"_el, "The selected header is missing or excluded."_el};
                }
            }
            auto patterns = el::List<el::PathGlobPatternPtr>{};
            for (const auto &pattern : entry->getStringList(cHeaderGlobs)) {
                const auto value = entry->valueOrThrow(cHeaderGlobs);
                try {
                    patterns.append(el::PathGlobPattern::compile(pattern));
                } catch (const el::ParseError &) {
                    throw el::conf::ConfError{
                        value,
                        "Invalid Reference Header Glob"_el,
                        "Expected a valid path glob."_el,
                        std::current_exception()};
                }
            }
            for (const auto &header : available) {
                for (const auto &pattern : patterns) {
                    if (pattern->fullMatch(header) && !headers.contains(header)) {
                        headers.append(header);
                        break;
                    }
                }
            }
            auto selected = el::StringList{};
            for (const auto &header : headers) {
                if (matches(entry->value(cExcludedHeaderGlobs), header)) {
                    continue;
                }
                if (claimed.contains(header)) {
                    throw el::conf::ConfError{
                        entry,
                        "Duplicate Reference Group Header"_el,
                        el::join("A header belongs to more than one reference group: "_el, header)};
                }
                claimed.insert(header);
                selected.append(header);
            }
            if (selected.isEmpty()) {
                throw el::conf::ConfError{
                    entry, "Empty Reference Group"_el, "The reference group has no matching headers."_el};
            }
            selected.sort([](const el::String &left, const el::String &right) {
                return el::toCaseFolded(left) < el::toCaseFolded(right);
            });
            _groups.append({page, entry->getText("title"_el), selected});
        }
    }
    for (const auto &header : available) {
        if (!claimed.contains(header)) {
            _unassigned.append(header);
        }
    }
    if (const auto manualPages = configuration->value(cManualReferencePages)) {
        for (const auto &value : manualPages->toValueList()) {
            const auto checked = referencePath(value);
            if (checked.suffix() != ".rst"_el || checked.name() == "index.rst"_el) {
                throw el::conf::ConfError{
                    value,
                    "Invalid Manual Reference Page"_el,
                    "Expected an RST page below the reference directory, excluding index.rst."_el};
            }
        }
    }
}

auto ReferenceDocumentation::referencePath(const el::conf::ValuePtr &value) const -> el::Path {
    const auto path = el::Path{value->asTextOrThrow()};
    if (path.isEmpty() || !path.isRelative()) {
        throw el::conf::ConfError{
            value, "Invalid Reference Page Path"_el, "Expected a nonempty relative page path."_el};
    }
    try {
        const auto result = _files.checked(_reference / path, true);
        if (!result.isRelativeTo(_reference)) {
            throw el::conf::ConfError{
                value, "Invalid Reference Page Path"_el, "The page must remain below the reference directory."_el};
        }
        return result;
    } catch (const el::ApplicationError &error) {
        throw el::conf::ConfError{
            value, "Invalid Reference Page Path"_el, error.description(), std::current_exception()};
    }
}

auto ReferenceDocumentation::title(const el::String &name) -> el::String {
    const auto words = el::StringList::fromSplit(name, el::CharSet{U'_'});
    auto result = el::StringList{};
    for (const auto &word : words) {
        result.append(el::toWordsFromCamelCase(word, el::Capitalization::Capitalized));
    }
    return result.join(" "_el);
}

auto ReferenceDocumentation::replaceInterface(const el::String &text, const el::String &body) -> el::String {
    static const auto reInterfaceHeading = el::RegEx::compile(R"re((?xm)
        ^ Interface \n # Exact section title at the start of a line
        ========= \n   # Exact underline and its terminating newline
    )re"_el);
    auto count = 0;
    auto prefix = el::String{};
    for (const auto &found : reInterfaceHeading->findAll(text)) {
        ++count;
        prefix = text.slice(el::ByteRange{el::ByteIndex{0}, el::ByteCount::fromInteger(found->range().end())});
    }
    if (count != 1) {
        throwError("Reference page requires exactly one final Interface section."_el);
    }
    if (body.trimmed().isEmpty()) {
        return prefix;
    }
    return el::join(prefix.trimmed(), "\n\n"_el, body.trimmed(), "\n"_el);
}

auto ReferenceDocumentation::replaceIndex(const el::String &text, el::StringList entries) -> el::String {
    const auto lines = cleanup::text::lines(text);
    auto result = el::StringList{};
    auto count = 0;
    entries.sort();
    for (auto index = el::ItemIndex{0}; index.isWithin(lines.count()); ++index) {
        const auto current = lines.getRef(index);
        result.append(current);
        if (current != ".. toctree::"_el) {
            continue;
        }
        ++count;
        while (
            (index + el::ItemCount{1}).isWithin(lines.count()) &&
            (lines.getRef(index + el::ItemCount{1}).isEmpty() ||
                lines.getRef(index + el::ItemCount{1}).startsWith(" "_el))) {
            ++index;
            if (lines.getRef(index).trimmed().startsWith(":"_el)) {
                result.append(lines.getRef(index));
            }
        }
        result.append(""_el);
        for (const auto &entry : entries) {
            result.append(el::join("    "_el, entry));
        }
    }
    if (count != 1) {
        throwError("Reference index requires exactly one toctree."_el);
    }
    return el::join(result.join("\n"_el).trimmed(), "\n"_el);
}

void ReferenceDocumentation::run(frame::TaskProgress progress) {
    _progress = std::move(progress);
    _progress.beginPhase("Discovering reference headers"_el);
    const auto scanned = scanHeaders();
    auto remaining = el::StringMap<int>{};
    const auto interfaces = collectInterfaces(scanned, remaining);
    auto outputs = el::StringMap<el::String>{};
    auto expected = el::StringSet{};
    _progress.beginPhase("Rendering reference pages"_el, _groups.count());
    for (const auto &group : _groups) {
        const auto key = group.page.toString();
        expected.insert(key);
        const auto path = _files.checked(_reference / group.page, true);
        outputs.set(path.toString(), renderInterface(group, interfaces.get(key, {}), remaining));
        _progress.advance();
    }
    for (const auto &manual : _manualPages) {
        expected.insert(manual);
    }
    for (const auto &[directory, entries] : collectIndexes(expected)) {
        outputs.set((el::Path{directory} / "index.rst"_el).toString(), renderIndex(directory, entries));
    }
    _progress.beginPhase("Publishing reference pages"_el, outputs.count());
    publish(outputs, expected);
}

auto ReferenceDocumentation::scanHeaders() const -> el::StringMap<el::StringList> {
    auto required = el::StringSet{};
    for (const auto &group : _groups) {
        for (const auto &header : group.headers) {
            required.insert(header);
            const auto path = el::Path{header};
            required.insert((path.parent() / el::join(path.stem(), "_fwd.hpp"_el)).toString());
        }
    }
    auto paths = el::PathList{};
    _source.walker().walkOrThrow(
        [&](const el::Path &path) -> el::PathWalkStatus {
            paths.append(path);
            return el::PathWalkStatus::Continue;
        },
        el::PathWalkOptions{}
            .setTypes(el::PathType::RegularFile)
            .setSymlinkMode(el::SymlinkMode::Skip)
            .setFilter([&](const el::Path &path, el::PathType) -> bool {
                return path.suffix() == ".hpp"_el && required.contains(path.toRelative(_source).toString());
            }));

    _progress.beginPhase("Scanning reference headers"_el, paths.count());
    auto scannedHeaders = el::StringMap<el::StringList>{};
    for (const auto &path : paths) {
        scannedHeaders.set(
            path.toRelative(_source).toString(), HeaderScanner{}.scan(_files.readWalkedTextFile(path).text()));
        _progress.advance();
    }
    return scannedHeaders;
}

auto ReferenceDocumentation::collectInterfaces(
    const el::StringMap<el::StringList> &scannedHeaders, el::StringMap<int> &remaining) const
    -> el::StringMap<el::List<el::StringList>> {
    auto interfaces = el::StringMap<el::List<el::StringList>>{};
    for (const auto &group : _groups) {
        const auto key = group.page.toString();
        auto headers = el::List<el::StringList>{};
        for (const auto &header : group.headers) {
            const auto found = scannedHeaders.get(header);
            if (!found) {
                throwError("Reference header disappeared before it could be scanned."_el, header);
            }
            auto scanned = *found;
            const auto headerPath = el::Path{header};
            const auto forward = headerPath.parent() / el::join(headerPath.stem(), "_fwd.hpp"_el);
            for (const auto &directive : scannedHeaders.get(forward.toString(), {})) {
                if (!scanned.contains(directive)) {
                    scanned.append(directive);
                }
            }
            for (const auto &directive : scanned) {
                remaining.set(directive, remaining.get(directive, 0) + 1);
            }
            headers.append(scanned);
        }
        interfaces.set(key, headers);
    }
    return interfaces;
}

auto ReferenceDocumentation::renderInterface(
    const ReferenceGroup &group, const el::List<el::StringList> &headers, el::StringMap<int> &remaining) const
    -> el::String {
    auto body = el::StringList{};
    for (const auto &header : headers) {
        auto entries = el::StringList{};
        for (const auto &directive : header) {
            const auto count = remaining.get(directive, 0) - 1;
            remaining.set(directive, count);
            if (count == 0) {
                entries.append(directive);
            }
        }
        const auto text = entries.join("\n"_el).trimmed();
        if (!text.isEmpty()) {
            body.append(text);
        }
    }
    const auto path = _files.checked(_reference / group.page, true);
    auto text = el::String{};
    if (path.info().isRegularFile()) {
        text = _files.readText(path);
    } else {
        auto heading = group.title;
        if (heading.isEmpty()) {
            heading = title(group.page.stem());
        }
        const auto rule = el::String::fromCharacter(U'*', heading.characterLength());
        text = el::join(
            ".. index::\n    single: "_el,
            heading,
            "\n\n"_el,
            rule,
            "\n"_el,
            heading,
            "\n"_el,
            rule,
            "\n\nInterface\n=========\n"_el);
    }
    try {
        return replaceInterface(text, body.join("\n"_el));
    } catch (const el::ApplicationError &error) {
        throw error.withSourcePath(path.toString());
    }
}

auto ReferenceDocumentation::collectIndexes(const el::StringSet &expected) const -> el::StringMap<el::StringList> {
    auto indexes = el::StringMap<el::StringList>{};
    indexes.set(_reference.toString(), el::StringList{});
    for (const auto &page : expected) {
        auto parent = (_reference / el::Path{page}).parent();
        auto entries = indexes.get(parent.toString(), {});
        entries.append(el::Path{page}.stem());
        indexes.set(parent.toString(), entries);
        while (parent != _reference) {
            const auto entry = el::join(parent.name(), "/index"_el);
            parent = parent.parent();
            entries = indexes.get(parent.toString(), {});
            if (!entries.contains(entry)) {
                entries.append(entry);
            }
            indexes.set(parent.toString(), entries);
        }
    }
    return indexes;
}

auto ReferenceDocumentation::renderIndex(const el::String &directory, const el::StringList &entries) const
    -> el::String {
    const auto path = el::Path{directory} / "index.rst"_el;
    auto text = el::String{};
    if (path.info().isRegularFile()) {
        text = _files.readText(path);
    } else {
        auto heading = el::String{"Reference"_el};
        if (el::Path{directory} != _reference) {
            heading = el::join(title(el::Path{directory}.name()), " Reference"_el);
        }
        const auto rule = el::String::fromCharacter(U'*', heading.characterLength());
        text = el::join(rule, "\n"_el, heading, "\n"_el, rule, "\n\n.. toctree::\n    :maxdepth: 1\n\n"_el);
    }
    try {
        return replaceIndex(text, entries);
    } catch (const el::ApplicationError &error) {
        throw error.withSourcePath(path.toString());
    }
}

void ReferenceDocumentation::publish(const el::StringMap<el::String> &outputs, const el::StringSet &expected) const {
    // Validate all pages before publishing any updates or removing orphans.
    for (const auto &[name, text] : outputs) {
        const auto path = el::Path{name};
        _files.createDirectories(path.parent());
        _files.updateText(path, text);
        _progress.advance();
    }
    _progress.beginPhase("Discovering obsolete reference pages"_el);
    const auto pages = _files.collectFilenamesOnly(_reference, ".rst"_el);
    _progress.beginPhase("Checking obsolete reference pages"_el, pages.count());
    for (const auto &path : pages) {
        if (path.name() != "index.rst"_el && !expected.contains(path.toRelative(_reference).toString())) {
            _files.checked(path).operations().removeOrThrow();
        }
        _progress.advance();
    }
}

void ReferenceDocumentation::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"Reference Documentation Update Failed"_el, description}.withSourcePath(path);
}

}
