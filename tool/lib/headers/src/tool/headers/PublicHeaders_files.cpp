// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicHeaders.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/StringTransformations.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::headers {

using namespace el::text::literals;

auto PublicHeaders::generated(const el::String &text) const -> bool {
    auto lines = el::StringSplitter{text, U'\n'};
    for (auto lineIndex = 0; lineIndex < 10 && !lines.isAtEnd(); ++lineIndex) {
        const auto line = lines.next().trimmed();
        if (_policy.generatedMarkers().anyOf(
                [&](const el::String &marker) -> bool { return line.startsWith(marker); })) {
            return true;
        }
        // Generated banners precede declarations; do not recognize text inside later raw string literals.
        if (!line.isEmpty() && !line.startsWith("//"_el) && !line.startsWith("#pragma once"_el)) {
            return false;
        }
    }
    return false;
}

auto PublicHeaders::outputPath(const el::Path &path) const -> el::Path {
    const auto absolute = _files.resolve(path);
    if (absolute == _files.projectRoot()) {
        HeaderPolicy::throwError("An output path leaves the project or targets its root."_el);
    }
    return absolute;
}

auto PublicHeaders::checkedOutput(const el::Path &path) const -> el::Path {
    return _files.checked(outputPath(path), true);
}

void PublicHeaders::ensureDirectory(const el::Path &path) const {
    const auto checked = _files.checked(path, true);
    if (checked.info().isDirectory()) {
        return;
    }
    auto current = _files.projectRoot();
    for (const auto &element : path.toRelativeOrThrow(_files.projectRoot()).elements()) {
        current.joinNative(element);
        if (!current.info().exists()) {
            _files.checked(current); // Revalidate at each actual creation boundary.
            current.operations().createDirectoryOrThrow();
        }
        if (!current.info().isDirectory()) {
            HeaderPolicy::throwError(el::StringFormat{"Output parent '{}' is not a directory."_el}.build(current));
        }
    }
}

void PublicHeaders::collectObsolete() {
    for (const auto &[name, file] : _existingFiles) {
        const auto &path = file.path();
        const auto &text = file.text();
        if (!_outputs.contains(name) && (path.isRelativeTo(_includeBase) || path.name() == "all.hpp"_el) &&
            generated(text)) {
            _obsolete.set(name, path);
        }
    }
}

void PublicHeaders::validatePlan() {
    const auto mappingPath = outputPath(_policy.mappingFile());
    const auto mappingName = mappingPath.toString();
    auto managedOutputs = FileSet{mappingName};
    if (_policy.generatesNamespaceHeaders()) {
        managedOutputs.insert((_sourceBase / _policy.namespacesHeader()).toString());
        managedOutputs.insert((_sourceBase / _policy.flattenHeader()).toString());
    }
    if (mappingPath.info().isRegularFile() && !_existingFiles.contains(mappingName)) {
        _existingFiles.set(mappingName, _files.readTextFile(mappingPath));
    }
    auto checkedParents = FileSet{};
    auto portablePaths = FileMap<el::String>{};
    for (const auto &[name, output] : _outputs) {
        validateOutput(name, output, managedOutputs, checkedParents);
        validatePathCollisions(output.path, portablePaths);
    }
}

void PublicHeaders::validateOutput(
    const el::String &name, const HeaderOutput &output, const FileSet &managedOutputs, FileSet &checkedParents) {
    const auto existing = _existingFiles.get(name);
    const auto &path = existing ? existing->path() : output.path;
    const auto parentPath = path.parent();
    if (checkedParents.tryInsert(parentPath.toString())) {
        const auto parent = _files.checked(parentPath, true);
        if (parent.info().exists() && !parent.info().isDirectory()) {
            HeaderPolicy::throwError(el::StringFormat{"Output parent '{}' is not a directory."_el}.build(parent));
        }
    }
    const auto info = path.info();
    if (info.exists()) {
        if (!info.isRegularFile()) {
            HeaderPolicy::throwError(el::StringFormat{"Output '{}' is not a regular file."_el}.build(name));
        }
        if (!managedOutputs.contains(name) && (!existing || !generated(existing->text()))) {
            HeaderPolicy::throwError(el::StringFormat{"Refusing to overwrite unmanaged header '{}'."_el}.build(name));
        }
    }
}

void PublicHeaders::validatePathCollisions(const el::Path &path, FileMap<el::String> &portablePaths) {
    auto component = path;
    while (component != _files.projectRoot()) {
        const auto spelling = component.toString();
        if (component != path && _outputs.contains(spelling)) {
            HeaderPolicy::throwError(el::StringFormat{"Output '{}' is also an output directory."_el}.build(component));
        }
        const auto folded = el::toAsciiLowercase(spelling);
        const auto previous = portablePaths.get(folded);
        if (previous) {
            if (*previous != spelling) {
                HeaderPolicy::throwError(
                    el::StringFormat{"Output paths '{}' and '{}' differ only by case."_el}.build(*previous, spelling));
            }
            break; // This component and all of its ancestors were already validated.
        }
        portablePaths.set(folded, spelling);
        component = component.parent();
    }
}

auto PublicHeaders::apply(const bool checkOnly) const -> std::size_t {
    _progress.beginPhase("Publishing headers"_el, _outputs.count());
    auto changes = _obsolete.count().toSizeT();
    auto preparedDirectories = FileSet{};
    for (const auto &[name, output] : _outputs) {
        if (const auto file = _existingFiles.get(name); file && _files.isUnchanged(*file, output.text)) {
            _progress.advance();
            continue;
        }
        const auto &path = output.path;
        const auto parent = path.parent();
        if (checkOnly && !parent.info().isDirectory()) {
            ++changes;
            _log->info("Missing output: "_el, name);
            _progress.advance();
            continue;
        }
        if (!checkOnly && preparedDirectories.tryInsert(parent.toString())) {
            ensureDirectory(parent);
        }
        if (_files.updateText(path, output.text, checkOnly)) {
            ++changes;
            if (checkOnly) {
                _log->info("Changed or missing output: "_el, name);
            }
        }
        _progress.advance();
    }
    _progress.beginPhase("Removing obsolete headers"_el);
    if (checkOnly) {
        for (const auto &[name, path] : _obsolete) {
            _log->info("Obsolete output: "_el, name);
        }
    }
    if (!checkOnly) {
        for (const auto &[name, path] : _obsolete) {
            _files.checked(path).operations().removeOrThrow();
        }
    }
    return changes;
}

}
