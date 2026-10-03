// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HeaderPolicy.hpp"

#include "HeaderPolicyConfig.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringPattern.hpp>

namespace tool::headers {

using namespace el::text::literals;

void HeaderPolicy::read(const el::conf::ValuePtr &configuration) {
    *this = HeaderPolicy{};
    _configuration = configuration;
    readPaths(configuration);
    readExclusions(configuration);
    readAliases(configuration);
    readGeneratedMarkers(configuration);
    const auto directory = configuration->valueOrThrow(config::cDirectory);
    if (!directory->isSectionWithNames() || directory->size() == 0) {
        throwError("Directory must contain at least one domain section."_el, directory);
    }
    for (const auto &section : *directory) {
        readDirectory(section, nullptr);
    }
}

void HeaderPolicy::readPaths(const el::conf::ValuePtr &configuration) {
    _sourceBase = el::Path{configuration->getTextOrThrow(config::cSourceBaseDirectory)};
    _includeBase = el::Path{configuration->getTextOrThrow(config::cIncludeBaseDirectory)};
    _mappingFile = el::Path{configuration->getTextOrThrow(config::cMappingFile)};
    _baseNamespace = configuration->getTextOrThrow(config::cBaseNamespace);
    _namespacesHeader = configuration->getTextOrThrow(config::cNamespacesHeader);
    _flattenHeader = configuration->getTextOrThrow(config::cMakeOneNamespaceHeader);
    for (
        const auto key :
        {config::cSourceBaseDirectory,
            config::cIncludeBaseDirectory,
            config::cMappingFile,
            config::cNamespacesHeader,
            config::cMakeOneNamespaceHeader}) {
        const auto value = configuration->valueOrThrow(key);
        validatePath(value->asTextOrThrow(), false, value);
    }
    if (_sourceBase == _includeBase || isChildPath(_sourceBase, _includeBase) ||
        isChildPath(_includeBase, _sourceBase)) {
        throwError(
            "Source and include directories must be distinct, non-overlapping trees."_el,
            configuration->valueOrThrow(config::cIncludeBaseDirectory));
    }
    for (const auto &base : {_sourceBase, _includeBase}) {
        if (_mappingFile == base || isChildPath(_mappingFile, base)) {
            throwError(
                "The mapping file must be outside the source and include trees."_el,
                configuration->valueOrThrow(config::cMappingFile));
        }
    }
    if (_namespacesHeader == _flattenHeader || !_namespacesHeader.endsWith(".hpp"_el) ||
        !_flattenHeader.endsWith(".hpp"_el)) {
        throwError(
            "Namespace outputs must be distinct .hpp headers."_el,
            configuration->valueOrThrow(config::cMakeOneNamespaceHeader));
    }
    validateName(_baseNamespace, true, configuration->valueOrThrow(config::cBaseNamespace));
}

void HeaderPolicy::readExclusions(const el::conf::ValuePtr &configuration) {
    _excludedDirectories = configuration->getStringList(config::cExcludedDirectories);
    for (const auto &name : _excludedDirectories) {
        validateName(name, false, configuration->valueOrThrow(config::cExcludedDirectories));
    }
    _excludedHeaders = configuration->getStringList(config::cExcludedHeaders);
    compilePatterns(_excludedHeaders, configuration->value(config::cExcludedHeaders));
    _excludedFromAll = configuration->getStringList(config::cExcludedFromAllHeaders);
    compilePatterns(_excludedFromAll, configuration->value(config::cExcludedFromAllHeaders));
    for (const auto &name : configuration->getStringList(config::cMergeableHeaderNames)) {
        validatePath(name, false, configuration->valueOrThrow(config::cMergeableHeaderNames));
        if (el::Path{name}.name() != name || !name.endsWith(".hpp"_el)) {
            throwError(
                "Mergeable header names must be .hpp filenames."_el,
                configuration->valueOrThrow(config::cMergeableHeaderNames));
        }
        _mergeableNames.insert(name);
    }
}

void HeaderPolicy::readAliases(const el::conf::ValuePtr &configuration) {
    for (const auto &name : configuration->getStringList(config::cDeclaredNamespaces)) {
        validateName(name, true, configuration->valueOrThrow(config::cDeclaredNamespaces));
        _declaredNamespaces.insert(el::join(_baseNamespace, "::"_el, name));
    }
    if (const auto aliases = configuration->value(config::cNamespaceAliases)) {
        if (!aliases->isSectionWithNames()) {
            throwError("Namespace Aliases must be a section."_el, aliases);
        }
        for (const auto &entry : *aliases) {
            const auto name = entry->name().asText();
            const auto target = entry->asTextOrThrow();
            validateName(name, false, entry);
            validateName(target, true, entry);
            _namespaceAliases.set(name, el::join(_baseNamespace, "::"_el, target));
        }
    }
    if (const auto aliases = configuration->value(config::cHeaderAliases)) {
        if (!aliases->isSectionWithNames()) {
            throwError("Header Aliases must be a section of named source/include pairs."_el, aliases);
        }
        for (const auto &entry : *aliases) {
            const auto source = entry->getTextOrThrow(cSourceHeader);
            const auto include = entry->getTextOrThrow(cIncludeHeader);
            validatePath(source, false, entry->valueOrThrow(cSourceHeader));
            validatePath(include, false, entry->valueOrThrow(cIncludeHeader));
            if (!source.endsWith(".hpp"_el) || !include.endsWith(".hpp"_el) || include == "all.hpp"_el) {
                throwError("Header aliases must name .hpp files and cannot recreate the universal all.hpp."_el, entry);
            }
            _headerAliases.set(source, _headerAliases.get(source, {}).insert(include));
        }
    }
}

void HeaderPolicy::readGeneratedMarkers(const el::conf::ValuePtr &configuration) {
    _generatedMarkers =
        configuration->getStringList(config::cGeneratedMarkers, el::StringList{"// !!! THIS IS A GENERATED FILE"_el});
    if (_generatedMarkers.isEmpty()) {
        throwError(
            "Generated Markers must contain at least one line-comment prefix."_el,
            configuration->valueOrThrow(config::cGeneratedMarkers));
    }
    for (const auto &marker : _generatedMarkers) {
        if (!marker.startsWith("//"_el) || marker.trimmed().length() <= el::ByteCount{2} || marker.contains("\n"_el) ||
            marker.contains("\r"_el)) {
            throwError(
                "Generated markers must be nonempty, single-line C++ comment prefixes."_el,
                configuration->valueOrThrow(config::cGeneratedMarkers));
        }
    }
}

auto HeaderPolicy::isChildPath(const el::Path &path, const el::Path &parent) -> bool {
    const auto name = path.toString();
    const auto prefix = parent.toString();
    return name.startsWith(prefix) &&
        name.slice(el::StringSide::Back, el::ByteIndex::end(prefix.length())).startsWith("/"_el);
}

void HeaderPolicy::readDirectory(const el::conf::ValuePtr &section, const DirectoryPolicy *parent) {
    if (!section->isSectionWithNames()) {
        throwError("Every directory entry must be a section."_el, section);
    }
    const auto name = section->name().asText();
    validateName(name, false, section);
    auto policy = DirectoryPolicy{};
    if (parent != nullptr) {
        policy = *parent;
        policy.directory = parent->directory / name;
    } else {
        policy.directory = el::Path{name};
        policy.sourceNamespace = el::join(_baseNamespace, "::"_el, name);
        policy.includedHeaders = el::StringList{"*.hpp"_el};
    }
    const auto nested = section->getBoolean(config::cNestedNamespace);
    if (nested && parent != nullptr) {
        policy.sourceNamespace = el::join(parent->sourceNamespace, "::"_el, name);
        policy.includedSymbols.clear();
        policy.targetNamespace = {};
    }
    if (section->hasValue(config::cFlatten)) {
        policy.flatten = section->getBooleanOrThrow(config::cFlatten);
    }
    // Destinations belong to each entry, independently of inherited publication and namespace settings.
    if (policy.flatten) {
        policy.targetDirectory = {};
    } else {
        policy.targetDirectory = policy.directory;
    }
    if (section->hasValue(config::cTargetDirectory)) {
        const auto target = section->getTextOrThrow(config::cTargetDirectory);
        validatePath(target, true, section->valueOrThrow(config::cTargetDirectory));
        policy.targetDirectory = el::Path{target};
    }
    if (section->hasValue(config::cTargetNamespace)) {
        policy.targetNamespace = section->getTextOrThrow(config::cTargetNamespace);
        validateName(policy.targetNamespace, false, section->valueOrThrow(config::cTargetNamespace));
    }
    policy.includedHeaders = section->getStringList(config::cIncludedHeaders, policy.includedHeaders);
    policy.excludedHeaders = section->getStringList(config::cExcludedHeaders, policy.excludedHeaders);
    policy.excludedFromFlattening =
        section->getStringList(config::cExcludedFromFlattening, policy.excludedFromFlattening);
    policy.excludedFromAll = section->getStringList(config::cExcludedFromAllHeaders, policy.excludedFromAll);
    policy.includedSymbols = section->getStringList(config::cIncludedSymbols, policy.includedSymbols);
    for (
        const auto key :
        {config::cIncludedHeaders,
            config::cExcludedHeaders,
            config::cExcludedFromFlattening,
            config::cExcludedFromAllHeaders}) {
        if (const auto value = section->value(key)) {
            compilePatterns(value->asStringListOrThrow(), value);
        }
    }
    // The inherited defaults must also be available in the compiled pattern cache.
    compilePatterns(policy.includedHeaders, section);
    for (const auto &symbol : policy.includedSymbols) {
        validateName(symbol, false, section);
    }
    if (!policy.includedSymbols.isEmpty() && (!policy.flatten || !policy.targetNamespace.isEmpty())) {
        throwError("Included Symbols requires Flatten: Yes and cannot be combined with Target Namespace."_el, section);
    }
    if (!_directories.tryInsert(policy.directory.toString(), policy)) {
        throwError("Duplicate source directory configuration."_el, section);
    }
    const auto fields = el::StringSet{
        config::cNestedNamespace,
        config::cFlatten,
        config::cTargetDirectory,
        config::cTargetNamespace,
        config::cIncludedHeaders,
        config::cExcludedHeaders,
        config::cExcludedFromFlattening,
        config::cExcludedFromAllHeaders,
        config::cIncludedSymbols};
    for (const auto &child : *section) {
        if (!fields.contains(child->name().asText())) {
            readDirectory(child, &policy);
        }
    }
}

void HeaderPolicy::compilePatterns(const el::StringList &patterns, const el::conf::ValuePtr &value) {
    for (const auto &pattern : patterns) {
        if (pattern.contains("/"_el)) {
            throwError(
                el::StringFormat{"Header glob '{}' must match filenames only; configure a directory section."_el}.build(
                    pattern),
                value);
        }
        if (!_patterns.contains(pattern)) {
            try {
                _patterns.set(pattern, std::make_shared<const el::StringPattern>(el::StringPattern::fromGlob(pattern)));
            } catch (const el::ParseError &) {
                throw el::conf::ConfError{
                    value,
                    "Invalid Public Header Pattern"_el,
                    "Expected a valid filename glob."_el,
                    std::current_exception()};
            }
        }
    }
}

auto HeaderPolicy::matches(const el::String &name, const el::StringList &patterns) const -> bool {
    for (const auto &pattern : patterns) {
        if (_patterns.get(pattern).value()->matches(name)) {
            return true;
        }
    }
    return false;
}

void HeaderPolicy::validatePath(const el::String &path, const bool allowEmpty, const el::conf::ValuePtr &value) {
    if (allowEmpty && path.isEmpty()) {
        return;
    }
    static const auto pattern = el::re::RegEx::compile("[A-Za-z0-9_.\\-]+(?:/[A-Za-z0-9_.\\-]+)*"_el);
    if (!pattern->fullMatch(path)) {
        throwError(el::StringFormat{"Invalid portable relative path: '{}'"_el}.build(path), value);
    }
    for (const auto &element : el::Path{path}.elements()) {
        if (element == "."_el || element == ".."_el) {
            throwError(el::StringFormat{"Dot components are not allowed in '{}'."_el}.build(path), value);
        }
    }
}

void HeaderPolicy::validateName(const el::String &name, const bool qualified, const el::conf::ValuePtr &value) {
    static const auto identifier = el::re::RegEx::compile("[A-Za-z_][A-Za-z0-9_]*"_el);
    static const auto namespaceName = el::re::RegEx::compile("[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*"_el);
    auto expression = identifier;
    if (qualified) {
        expression = namespaceName;
    }
    if (!expression->fullMatch(name)) {
        throwError(el::StringFormat{"Invalid C++ name: '{}'"_el}.build(name), value);
    }
}

void HeaderPolicy::throwError(const el::String &description, const el::conf::ValuePtr &value) {
    if (value) {
        throw el::conf::ConfError{value, "Invalid Public Header Configuration"_el, description};
    }
    throw el::ApplicationError{"Public Header Generation Failed"_el, description};
}

}
