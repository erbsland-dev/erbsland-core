// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Policy.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/StringFormat.hpp>

namespace tool::anti_patterns {

using namespace el::text::literals;

void Policy::read(
    const el::conf::ValuePtr &configuration,
    [[maybe_unused]] const frame::ProjectFiles &files,
    const RuleRegistry &rules) {
    _sourceDirectories = configuration->valueOrThrow("source_directories"_el);
    _directoryPatterns.clear();
    _exceptions.clear();
    readSources(configuration);
    readExceptions(configuration, rules);
}

void Policy::readSources(const el::conf::ValuePtr &configuration) {
    _suffixes = configuration->getStringListOrThrow(cFileSuffixes);
    if (_suffixes.isEmpty()) {
        throw el::conf::ConfError{
            configuration->valueOrThrow(cFileSuffixes),
            "Invalid Anti-Pattern Configuration"_el,
            "File Suffixes must not be empty."_el};
    }
    for (const auto &suffix : _suffixes) {
        if (!suffix.startsWith("."_el) || suffix.length() < el::ByteCount{2} || suffix.contains("/"_el)) {
            throw el::conf::ConfError{
                configuration->valueOrThrow(cFileSuffixes),
                "Invalid Anti-Pattern Configuration"_el,
                "Invalid file suffix."_el};
        }
    }
    for (const auto &value : _sourceDirectories->toValueList()) {
        const auto directory = value->asTextOrThrow();
        const auto relative = el::Path{directory.trimmed()};
        if (directory.trimmed().isEmpty() || !relative.isRelative() || relative.elements().contains(".."_el)) {
            throw el::conf::ConfError{
                value,
                "Invalid Anti-Pattern Configuration"_el,
                "Source directory patterns must be safe project-relative globs."_el};
        }
        try {
            _directoryPatterns.set(directory, el::PathGlobPattern::compile(directory));
        } catch (const el::ParseError &) {
            throw el::conf::ConfError{
                value,
                "Invalid Source Directory Glob"_el,
                "Invalid source directory glob."_el,
                std::current_exception()};
        }
    }
    if (_directoryPatterns.count().isZero()) {
        throw el::conf::ConfError{
            _sourceDirectories, "Invalid Anti-Pattern Configuration"_el, "Source Directories must not be empty."_el};
    }
}

void Policy::readExceptions(const el::conf::ValuePtr &configuration, const RuleRegistry &rules) {
    if (const auto entries = configuration->getSectionList("exception"_el)) {
        for (const auto &entry : *entries) {
            const auto paths = entry->valueOrThrow("path"_el)->toValueList();
            for (const auto &path : paths) {
                auto exception = ExceptionFromRule{};
                exception.path = path->asTextOrThrow().trimmed();
                exception.reason = entry->getTextOrThrow(cReason).trimmed();
                exception.rules = entry->getStringListOrThrow(cRules);
                if (exception.path.isEmpty()) {
                    throw el::conf::ConfError{
                        path, "Invalid Exception Path"_el, "Exception Path must not be empty."_el};
                }
                if (exception.reason.isEmpty()) {
                    throw el::conf::ConfError{
                        entry->valueOrThrow(cReason),
                        "Invalid Exception Reason"_el,
                        "Exception Reason must not be empty."_el};
                }
                if (exception.rules.isEmpty()) {
                    throw el::conf::ConfError{
                        entry->valueOrThrow(cRules),
                        "Invalid Exception Rules"_el,
                        "Exception Rules must not be empty."_el};
                }
                const auto lexical = el::Path{exception.path};
                if (!lexical.isRelative() || lexical.elements().contains(".."_el)) {
                    throw el::conf::ConfError{
                        path,
                        "Invalid Exception Path"_el,
                        "Exception Path must be a project-relative glob without '..'."_el};
                }
                try {
                    exception.pattern = el::PathGlobPattern::compile(exception.path);
                } catch (const el::ParseError &) {
                    throw el::conf::ConfError{
                        path, "Invalid Exception Path"_el, "Expected a valid path glob."_el, std::current_exception()};
                }
                auto uniqueRules = el::StringList{};
                for (const auto &identifier : exception.rules) {
                    if (rules.find(identifier) == nullptr) {
                        throw el::conf::ConfError{
                            entry->valueOrThrow(cRules),
                            "Invalid Exception Rules"_el,
                            el::StringFormat{"Unknown anti-pattern rule: {}"_el}.build(identifier)};
                    }
                    if (uniqueRules.contains(identifier)) {
                        throw el::conf::ConfError{
                            entry->valueOrThrow(cRules),
                            "Invalid Exception Rules"_el,
                            "Duplicate exception path and rule."_el};
                    }
                    uniqueRules.append(identifier);
                    for (const auto &previous : _exceptions) {
                        if (previous.path == exception.path && previous.rules.contains(identifier)) {
                            throw el::conf::ConfError{
                                entry->valueOrThrow(cRules),
                                "Invalid Exception Rules"_el,
                                "Duplicate exception path and rule."_el};
                        }
                    }
                }
                _exceptions.append(std::move(exception));
            }
        }
    }
}

auto Policy::roots(const frame::ProjectFiles &files) const -> el::PathList {
    auto unique = el::StringMap<el::Path>{};
    for (const auto &[text, pattern] : _directoryPatterns) {
        auto matched = false;
        files.projectRoot().walker().walkOrThrow(
            [&](const el::Path &path, [[maybe_unused]] const el::PathInfo &info) -> el::PathWalkStatus {
                matched = true;
                unique.set(path.toString(), path);
                return el::PathWalkStatus::Continue;
            },
            el::PathWalkOptions{}
                .setGlobPattern(pattern)
                .setTypes(el::PathType::Directory)
                .setSymlinkMode(el::SymlinkMode::Skip));
        if (!matched) {
            throw el::conf::ConfError{
                _sourceDirectories,
                "Unmatched Source Directory Glob"_el,
                el::StringFormat{"Source directory glob matches no directories: {}"_el}.build(text)};
        }
    }
    auto result = el::PathList{};
    for (const auto &[text, path] : unique) {
        result.append(path);
    }
    return result;
}

auto Policy::accepts(const el::Path &path) const -> bool {
    return _suffixes.contains(path.suffix());
}

auto Policy::reason(const el::Path &path, const el::String &rule) const -> el::String {
    for (const auto &exception : _exceptions) {
        if (exception.rules.contains(rule) && exception.pattern->fullMatch(path)) {
            return exception.reason;
        }
    }
    return {};
}

}
