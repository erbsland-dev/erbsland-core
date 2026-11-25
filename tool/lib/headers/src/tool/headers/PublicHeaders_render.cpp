// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicHeaders.hpp"

#include "HeaderPolicyConfig.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/StringTransformations.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::headers {

using namespace el::text;
using namespace el::text::literals;

auto PublicHeaders::isChildNamespace(const el::String &name, const el::String &parent) -> bool {
    return name.startsWith(parent) &&
        name.slice(el::StringSide::Back, el::ByteIndex::end(parent.length())).startsWith("::"_el);
}

void PublicHeaders::preserveAliases() {
    for (const auto &[source, aliases] : _policy.headerAliases()) {
        auto header = _headers.get(source);
        if (!header) {
            HeaderPolicy::throwError(
                el::StringFormat{"Header aliases reference unpublished source '{}'."_el}.build(source),
                _policy.configuration()->valueOrThrow(config::cHeaderAliases));
        }
        for (const auto &alias : aliases) {
            header->aliases.insert(alias);
        }
        _headers.set(source, *header);
    }
    static const auto cInclude = el::RegEx::compile("^#include \"([^\"]+)\"$"_el);
    for (const auto &[name, file] : _existingFiles) {
        const auto &path = file.path();
        const auto &text = file.text();
        if (!path.isRelativeTo(_includeBase) || path == _includeBase / "all.hpp"_el || !generated(text)) {
            continue;
        }
        auto lines = el::StringSplitter{text, U'\n'};
        while (!lines.isAtEnd()) {
            const auto line = lines.next().trimmed();
            if (!line.startsWith("#include "_el)) {
                continue;
            }
            if (const auto match = cInclude->fullMatch(line)) {
                const auto target = (path.parent() / match->content(1)).resolveOrThrow(el::PathResolveMode::Lexical);
                if (target.isRelativeTo(_sourceBase)) {
                    const auto source = target.toRelativeOrThrow(_sourceBase).toString();
                    if (auto header = _headers.get(source)) {
                        const auto alias = path.toRelativeOrThrow(_includeBase).toString();
                        HeaderPolicy::validatePath(alias);
                        header->aliases.insert(alias);
                        _headers.set(source, *header);
                    }
                }
            }
        }
    }
}

void PublicHeaders::createUmbrellas() {
    static const auto cUmbrellaNameFormat = el::StringFormat{"all_{}.hpp"_el};
    for (const auto &[directory, policy] : _policy.directories()) {
        auto includes = el::StringList{};
        for (const auto &[source, header] : _headers) {
            const auto sourcePath = el::Path{source};
            if (!header.umbrella && !header.excludeFromAll && sourcePath.parent() == policy.directory) {
                includes.append(sourcePath.name());
            }
        }
        const auto source = (policy.directory / "all.hpp"_el).toString();
        addOutput(_sourceBase / source, "headers/umbrella.cpp"_el, render::Context{}.set(cIncludesKey, includes));
        const auto canonical = cUmbrellaNameFormat.build(directory.replacedAll("/"_el, "_"_el));
        auto aliases = el::StringSet{source};
        const auto parent = _policy.directories().get(policy.directory.parent().toString());
        if (!policy.targetDirectory.isEmpty() && (!parent || parent->sourceNamespace != policy.sourceNamespace)) {
            aliases.insert((policy.targetDirectory / "all.hpp"_el).toString());
        }
        _headers.set(source, PublicHeader{source, canonical, std::move(aliases), true, true});
    }
}

void PublicHeaders::createNamespaces() {
    auto namespaces = _policy.declaredNamespaces();
    auto imports = el::StringSet{};
    auto aliases = _policy.namespaceAliases();
    for (const auto &[path, directory] : _policy.directories()) {
        namespaces.insert(directory.sourceNamespace);
        if (!directory.targetNamespace.isEmpty()) {
            const auto previous = aliases.get(directory.targetNamespace);
            if (previous && *previous != directory.sourceNamespace) {
                HeaderPolicy::throwError(
                    el::StringFormat{"Conflicting namespace alias '{}'."_el}.build(directory.targetNamespace));
            }
            aliases.set(directory.targetNamespace, directory.sourceNamespace);
        } else if (directory.flatten && directory.includedSymbols.isEmpty()) {
            imports.insert(directory.sourceNamespace);
        }
    }
    auto aliasValues = render::ValueMap{};
    for (const auto &[alias, target] : aliases) {
        namespaces.insert(target);
        aliasValues.set(alias, target);
    }
    for (const auto &[alias, target] : aliases) {
        const auto name = el::join(_policy.baseNamespace(), "::"_el, alias);
        for (const auto &declared : namespaces) {
            if (declared == name || isChildNamespace(declared, name)) {
                HeaderPolicy::throwError(
                    el::StringFormat{"Namespace alias '{}' conflicts with a declared namespace."_el}.build(alias));
            }
        }
    }
    addOutput(
        _sourceBase / _policy.namespacesHeader(),
        "headers/namespaces.cpp"_el,
        render::Context{}.set("namespaces"_el, namespaces));
    // Publish the generated declaration header even on the first run.
    const auto namespacePath = el::Path{_policy.namespacesHeader()};
    const auto policy = _policy.directories().get(namespacePath.parent().toString());
    if (!_headers.contains(_policy.namespacesHeader()) && policy &&
        _policy.matches(namespacePath.name(), policy->includedHeaders) &&
        !_policy.matches(namespacePath.name(), _policy.excludedHeaders()) &&
        !_policy.matches(namespacePath.name(), policy->excludedHeaders)) {
        auto target = policy->targetDirectory;
        if (!policy->includedSymbols.isEmpty() ||
            _policy.matches(namespacePath.name(), policy->excludedFromFlattening)) {
            target = policy->directory;
        }
        _headers.set(
            _policy.namespacesHeader(),
            PublicHeader{
                _policy.namespacesHeader(),
                (_includeBase / target).joinedNative(namespacePath.name()).toRelativeOrThrow(_includeBase).toString(),
                el::StringSet{_policy.namespacesHeader()},
                false,
                _policy.matches(namespacePath.name(), _policy.excludedFromAll()) ||
                    _policy.matches(namespacePath.name(), policy->excludedFromAll)});
    }
    const auto destination = _sourceBase / _policy.flattenHeader();
    auto includes =
        el::StringList{(_sourceBase / _policy.namespacesHeader()).toRelativeOrThrow(destination.parent()).toString()};
    for (const auto &source : _forwardIncludes) {
        includes.append((_sourceBase / source).toRelativeOrThrow(destination.parent()).toString());
    }
    auto forwards = render::ValueMap{};
    for (const auto &[name, declarations] : _forwardDeclarations) {
        forwards.set(name, declarations);
    }
    auto exports = render::ValueMap{};
    for (const auto &[name, symbols] : _selectedExports) {
        exports.set(name, symbols);
    }
    addOutput(
        destination,
        "headers/exports.cpp"_el,
        render::Context{}
            .set("base_namespace"_el, _policy.baseNamespace())
            .set(cIncludesKey, includes)
            .set("forwards"_el, forwards)
            .set("imports"_el, imports)
            .set("exports"_el, exports)
            .set("aliases"_el, aliasValues));
}

void PublicHeaders::createWrappers() {
    auto wrappers = el::StringMap<el::StringSet>{};
    auto foldedPaths = FileMap<el::String>{};
    for (const auto &[source, header] : _headers) {
        auto targets = header.aliases;
        targets.insert(header.canonical);
        for (const auto &target : targets) {
            HeaderPolicy::validatePath(target);
            const auto folded = el::toAsciiLowercase(target);
            const auto previous = foldedPaths.get(folded);
            if (previous && *previous != target) {
                HeaderPolicy::throwError(
                    el::StringFormat{"Public include paths '{}' and '{}' differ only by case."_el}.build(
                        *previous, target));
            }
            foldedPaths.set(folded, target);
            wrappers.set(target, wrappers.get(target, {}).insert(source));
        }
    }
    for (const auto &[target, sources] : wrappers) {
        if (sources.count() > el::ItemCount{1} && !_policy.mergeableNames().contains(el::Path{target}.name())) {
            HeaderPolicy::throwError(
                el::StringFormat{"Public include conflict at '{}':\n{}"_el}.build(
                    target, sources.toStringList().join("\n"_el)));
        }
        const auto destination = _includeBase / target;
        const auto parent = destination.parent();
        auto includes = el::StringList{};
        for (const auto &source : sources) {
            includes.append((_sourceBase / source).toRelativeOrThrow(parent).toString());
        }
        addOutput(destination, "headers/wrapper.cpp"_el, render::Context{}.set(cIncludesKey, includes));
    }
}

void PublicHeaders::createMapping() {
    auto text = el::StringEditor{};
    for (const auto &[source, header] : _headers) {
        text.append(source).append(";"_el).append(header.canonical).append("\n"_el);
        for (const auto &alias : header.aliases) {
            if (alias != header.canonical) {
                text.append(source).append(";"_el).append(alias).append("\n"_el);
            }
        }
    }
    const auto path = outputPath(_policy.mappingFile());
    const auto name = path.toString();
    if (_renderJobs.contains(name) || !_outputs.tryInsert(name, HeaderOutput{path, text})) {
        HeaderPolicy::throwError("The mapping file conflicts with another generated output."_el);
    }
}

void PublicHeaders::addOutput(const el::Path &path, const el::String &layout, render::Context context) {
    context.set("generated_marker"_el, _policy.generatedMarkers().first());
    const auto destination = outputPath(path);
    if (!_renderJobs.tryInsert(destination.toString(), HeaderRenderJob{destination, layout, std::move(context)})) {
        HeaderPolicy::throwError(el::StringFormat{"Multiple outputs target '{}'."_el}.build(path));
    }
}

void PublicHeaders::renderOutputs(el::WorkerPool &workers) {
    _renderer = frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp);
    auto inputs = std::vector<HeaderRenderJob>{};
    for (const auto &[path, job] : _renderJobs) {
        inputs.push_back(job);
    }
    _progress.beginPhase("Rendering headers"_el, el::ItemCount::fromSizeT(inputs.size()));
    auto outputs = workers
                       .map(
                           inputs,
                           [renderer = _renderer, progress = _progress](const HeaderRenderJob &job) -> HeaderOutput {
                               auto output = HeaderOutput{
                                   job.path, el::join(renderer->render(job.layout, job.context).trimmed(), "\n"_el)};
                               progress.advance();
                               return output;
                           })
                       .get();
    for (auto &output : outputs) {
        const auto name = output.path.toString();
        _outputs.set(name, std::move(output));
    }
}

}
