// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CryptologyVectorsTool.hpp"

#include "impl/VectorConverter.hpp"
#include "impl/VectorDownloader.hpp"

#include "../frame/TextChecksum.hpp"
#include "../frame/ToolApplication.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringTransformations.hpp>

namespace tool::test_data {

using namespace el::text::literals;

void CryptologyVectorsTool::execute() {
    auto &files = frame::app().files();
    auto downloader = std::unique_ptr<impl::VectorDownloader>{};
    if (!_sourceDirectory) {
        downloader = std::make_unique<impl::VectorDownloader>(_maximumBytes, _timeout);
    }
    for (const auto &source : _sources) {
        if (!selected(source.descriptor)) {
            continue;
        }
        log()->info("Importing "_el, source.descriptor.filename);
        auto bytes = el::ByteBlock{};
        if (_sourceDirectory) {
            bytes = files.readData(*_sourceDirectory / source.descriptor.filename);
        } else {
            bytes = downloader->download(source.url);
        }
        if (bytes.length() > _maximumBytes) {
            throw el::ApplicationError{"Source Too Large"_el, "The source exceeds maximum_source_bytes."_el}
                .withSourcePath(source.descriptor.filename);
        }
        auto checksum = el::ByteBlock{};
        if (source.descriptor.filename.endsWith(".zip"_el)) {
            auto hasher = el::Hasher{el::HashAlgorithm::Sha2_256};
            hasher.update(bytes);
            checksum = hasher.finalize();
        } else {
            checksum = frame::calculateTextChecksum(bytes, el::HashAlgorithm::Sha2_256);
        }
        if (checksum != source.checksum) {
            throw el::ApplicationError{
                "Source Checksum Mismatch"_el, "The source does not match its configured SHA-256 digest."_el}
                .withSourcePath(source.url.toString());
        }
        try {
            auto converter = impl::VectorConverter{_renderer, source.descriptor, source.revision};
            for (const auto &[name, text] : converter.convert(bytes)) {
                _outputs.set(name, text);
            }
        } catch (const el::RuntimeError &) {
            throw el::ApplicationError{
                "Cryptology Source Conversion Failed"_el,
                "The verified source could not be converted into the expected fixtures."_el,
                std::current_exception()}
                .withSourcePath(source.url.toString());
        }
    }
    prepareMetadata();
    publish();
}

void CryptologyVectorsTool::prepareMetadata() {
    auto &files = frame::app().files();
    auto responses = _outputs;
    if (_outputDirectory.info().isDirectory()) {
        for (const auto &path : files.collectFilenamesOnly(_outputDirectory, ".rsp"_el)) {
            const auto name = path.toRelativeOrThrow(_outputDirectory).toString();
            const auto retained = files.readTextFile(path);
            const auto proposed = responses.get(name);
            if (!proposed || files.isUnchanged(retained, *proposed)) {
                responses.set(name, retained.text());
            }
        }
    }
    auto rows = el::render::ValueList{};
    for (const auto &[name, text] : responses) {
        auto digest = frame::calculateTextChecksum(text, el::HashAlgorithm::Sha2_256);
        const auto path = _outputDirectory / name;
        if (path.info().isRegularFile() && files.isUnchanged(files.readTextFile(path), text)) {
            digest = files.textChecksum(path, el::HashAlgorithm::Sha2_256);
        }
        const auto [records, groups] = impl::VectorConverter::counts(name, text);
        auto row = el::render::ValueMap{};
        row.set("path"_el, name);
        row.set(cChecksumKey, el::toAsciiLowercase(el::String::fromByteBlock(digest)));
        row.set("records"_el, records);
        row.set("groups"_el, groups);
        rows.append(row);
    }
    auto context = el::render::Context{};
    context.set("rows"_el, rows);
    _outputs.set("manifest.txt"_el, _renderer->render("cryptology_vectors/manifest.tpl"_el, context));
    auto sources = el::render::ValueList{};
    for (const auto &source : _sources) {
        auto value = el::render::ValueMap{};
        value.set("filename"_el, source.descriptor.filename);
        value.set("url"_el, source.url.toString());
        value.set(cChecksumKey, el::toAsciiLowercase(el::String::fromByteBlock(source.checksum)));
        value.set("revision"_el, source.revision);
        value.set(cAttributionKey, source.configuration->getTextOrThrow(cAttributionKey));
        sources.append(value);
    }
    context.set("sources"_el, sources);
    _outputs.set("README.rst"_el, _renderer->render("cryptology_vectors/provenance.tpl"_el, context));
}

}
