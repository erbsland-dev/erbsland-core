// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ElclFormatTool.hpp"

#include "FormatConfig.hpp"

#include "impl/FilePermissions.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/syntax/Formatter.hpp>
#include <erbsland/conf/syntax/Parser.hpp>
#include <erbsland/Diagnostic.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionFlag.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringDecoder.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringMap.hpp>

#include <algorithm>

namespace tool::elcl_format {

using namespace el::text::literals;
auto ElclFormatTool::identifier() const -> el::String {
    return "elcl_format"_el;
}

auto ElclFormatTool::title() const -> el::String {
    return "Format ELCL configuration files"_el;
}

auto ElclFormatTool::description() const -> el::String {
    return "Reformat files using the nearest ancestor elcl_format.elcl, or built-in defaults. Works outside the project."_el;
}

auto ElclFormatTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({cPaths})
        .setType(el::OptionType::Text)
        .setFlag(el::OptionFlag::Required)
        .setMaximum(el::ArgumentCount{65535})
        .setFlag(el::OptionFlag::Greedy)
        .setValueName("path"_el)
        .setHelpTitle("Files or directories to format."_el);
    module->addOption({"--recursive"_el, "-r"_el, cRecursive})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Scan directories recursively for .elcl files."_el);
    module->addOption({"--output-only"_el, cOutputOnly})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Print exactly one selected file to stdout without changing it."_el);
    return module;
}

void ElclFormatTool::configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &values) {
    _options = values;
    _requested = values->getTextList(cPaths);
    _recursive = values->getFlag(cRecursive);
    _outputOnly = values->getFlag(cOutputOnly);
    if (const auto option = values->value("config"_el)) {
        try {
            const auto path = el::Path{option->getText()};
            if (path.isEmpty()) {
                throw el::OptionError{
                    "Invalid Formatter Configuration"_el, "The configuration path must not be empty."_el, option};
            }
            _configuration = path.toAbsoluteOrThrow();
            if (!_configuration.info().isRegularFile()) {
                throw el::OptionError{
                    "Invalid Formatter Configuration"_el, "Select an existing regular configuration file."_el, option};
            }
        } catch (const el::PathError &) {
            throw el::OptionError{
                "Invalid Formatter Configuration"_el,
                "The configuration path could not be resolved."_el,
                option,
                std::current_exception()};
        }
    }
}

void ElclFormatTool::report(const el::Path &file, const el::String &message, const bool error) {
    if (error) {
        _failed = true;
    }
    el::stdErr()->writeLine(el::StringFormat{"{}: {}"_el}.build(file.toString(), message));
}

void ElclFormatTool::discover() {
    for (const auto &requested : _requested) {
        try {
            if (requested.isEmpty()) {
                throw el::OptionError{
                    "Invalid ELCL Input"_el, "Input paths must not be empty."_el, _options->value(cPaths)};
            }
            const auto path = el::Path{requested}.toAbsoluteOrThrow();
            const auto info = path.info();
            if (info.isSymlink() || info.isReparsePoint()) {
                throw el::OptionError{
                    "Invalid ELCL Input"_el,
                    "Explicit symbolic links and reparse points are not supported."_el,
                    _options->value(cPaths)};
            }
            if (info.isRegularFile()) {
                _files.push_back(info.resolvedPath());
            } else if (info.isDirectory()) {
                auto options = el::PathWalkOptions{}.setSymlinkMode(el::SymlinkMode::Skip);
                options.setDescendFilter(
                    [this, path](const el::Path &entry, el::PathType) { return _recursive || entry == path; });
                path.walker().walkOrThrow(
                    el::PathInfoWalkFn{[this](const el::Path &entry, const el::PathInfo &entryInfo) {
                        if (entryInfo.isRegularFile() && entry.name().endsWith(".elcl"_el)) {
                            _files.push_back(entryInfo.resolvedPath());
                        }
                        return el::PathWalkStatus::Continue;
                    }},
                    options);
            } else {
                throw el::OptionError{
                    "Invalid ELCL Input"_el,
                    "Input is not an accessible regular file or directory."_el,
                    _options->value(cPaths)};
            }
        } catch (const el::PathError &) {
            throw el::OptionError{
                "Invalid ELCL Input"_el,
                "An input path could not be inspected."_el,
                _options->value(cPaths),
                std::current_exception()};
        }
    }
    std::sort(_files.begin(), _files.end(), [](const auto &a, const auto &b) { return a.toString() < b.toString(); });
    _files.erase(std::unique(_files.begin(), _files.end()), _files.end());
    if (_files.empty()) {
        throw el::OptionError{
            "No ELCL Files Selected"_el,
            "Provide at least one file or a directory containing .elcl files."_el,
            _options->value(cPaths)};
    }
}

auto ElclFormatTool::findConfiguration(const el::Path &file) const -> el::Path {
    if (!_configuration.isEmpty()) {
        return _configuration;
    }
    auto directory = file.parent();
    while (!directory.isEmpty()) {
        const auto candidate = directory / "elcl_format.elcl"_el;
        if (candidate.info().exists()) {
            return candidate;
        }
        const auto parent = directory.parent();
        if (parent == directory) {
            break;
        }
        directory = parent;
    }
    return {};
}

void ElclFormatTool::execute() {
    discover();
    if (_outputOnly && (_files.size() != 1 || _failed)) {
        throw el::OptionError{
            "Invalid Output-Only Selection"_el,
            "--output-only requires exactly one successfully discovered file."_el,
            _options->value(cOutputOnly)};
    }
    auto configurations = el::StringMap<el::conf::syntax::Format>{};
    auto selected = el::StringList{};
    for (const auto &file : _files) {
        const auto path = findConfiguration(file);
        const auto key = path.toString();
        if (!configurations.contains(key)) {
            if (path.isEmpty()) {
                configurations.set(key, el::conf::syntax::Format{});
            } else {
                configurations.set(key, FormatConfig{path}.read());
            }
        }
        selected.append(key);
    }
    for (std::size_t i = 0; i < _files.size(); ++i) {
        try {
            formatFile(_files[i], configurations.get(selected.getRef(el::ItemIndex::fromSizeT(i))).value());
        } catch (const el::RuntimeError &error) {
            report(_files[i], error.diagnostic()->toString());
        }
    }
    if (_failed) {
        throw el::ApplicationError{"ELCL Formatting Reported Errors"_el, "See the per-file diagnostics above."_el};
    }
}

void ElclFormatTool::formatFile(const el::Path &file, const el::conf::syntax::Format &format) {
    auto info = file.info();
    info.reload(el::PathInfoPart::All);
    const auto identity = info.fileIdentity();
    const auto original =
        file.content().readDataOrThrow(el::PathReadDataOptions{}.setSymlinkMode(el::SymlinkMode::Skip));
    const auto text = el::StringDecoder{original}.decode(
        el::StringEncoding::Utf8, el::StringBomMode::Automatic, el::EncodingMode::Strict);
    auto bom = el::StringBomMode::Automatic;
    if (original.length() > text.length()) {
        bom = el::StringBomMode::Require;
    }
    const auto document = el::conf::syntax::Parser{}.parseText(text, el::conf::Source::fromFile(file)->identifier());
    const auto result = el::conf::syntax::Formatter{format}.format(document);
    for (const auto &diagnostic : result.diagnostics()) {
        report(
            file,
            el::StringFormat{"{}: {}"_el}.build(diagnostic.location.toString(), diagnostic.message),
            !diagnostic.warning);
    }
    if (_outputOnly) {
        if (bom == el::StringBomMode::Require) {
            el::stdOut()->write(el::Char{0xfeff});
        }
        if (!el::stdOut()->write(result.text()).isSuccess()) {
            throw el::ApplicationError{"Could Not Write Formatted Output"_el, file.toString()};
        }
        el::stdOut()->flush();
        return;
    }
    const auto bytes = el::StringEncoder{result.text()}.encode(el::StringEncoding::Utf8, bom);
    if (bytes == original) {
        return;
    }
    file.content().replaceDataOrThrow(
        bytes,
        el::PathReplaceDataOptions{}.setBeforeCommit(
            [file, identity, original](const el::Path &temporary, const el::Path &) {
                auto current = file.info();
                current.reload(el::PathInfoPart::All);
                if (!current.isRegularFile() || current.fileIdentity() != identity ||
                    file.content().readDataOrThrow(el::PathReadDataOptions{}.setSymlinkMode(el::SymlinkMode::Skip)) !=
                        original) {
                    throw el::ApplicationError{"Input Changed During Formatting"_el, file.toString()};
                }
                impl::copyFilePermissions(file, temporary);
            }));
}

}
