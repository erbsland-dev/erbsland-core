// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UcdDataset.hpp"

#include "../frame/TextChecksum.hpp"
#include "../frame/ToolApplication.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/conf/Name.hpp>
#include <erbsland/conf/NamePath.hpp>
#include <erbsland/EncodingError.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/StringDecoder.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringSplitter.hpp>

#include <utility>

namespace tool::unicode {

using namespace el::text::literals;

auto UcdDataset::filenames() -> el::StringList {
    return el::StringList{
        "Blocks.txt"_el,
        "DerivedCoreProperties.txt"_el,
        "DerivedJoiningType.txt"_el,
        "DerivedNormalizationProps.txt"_el,
        "HangulSyllableType.txt"_el,
        "PropList.txt"_el,
        "Scripts.txt"_el,
        "UnicodeData.txt"_el,
    };
}

UcdDataset::UcdDataset(const el::Path &directory, const el::String &version, el::StringList requiredFiles) :
    _directory{frame::app().files().checked(directory)} {
    const auto manifest = frame::app().files().readConfiguration(_directory / "manifest.elcl"_el, false);
    const auto manifestVersion = manifest->valueOrThrow("dataset.unicode_version"_el);
    if (manifestVersion->asTextOrThrow() != version) {
        throwDatasetError(
            "The UCD manifest has an unexpected Unicode version."_el,
            _directory / "manifest.elcl"_el,
            {},
            manifestVersion->location().codeLocation());
    }
    const auto hashes = manifest->valueOrThrow("files"_el);
    for (const auto &name : requiredFiles) {
        const auto path = sourcePath(name);
        const auto bytes = frame::app().files().readData(path);
        const auto digest = frame::calculateTextChecksum(bytes, el::HashAlgorithm::Sha3_256);
        const auto checksum =
            el::text::base_n::BaseNEncoder{digest, el::text::base_n::BaseNFormat::base16()}.toString();
        const auto expected =
            hashes->valueOrThrow(el::conf::NamePath{el::conf::Name::createText(name)})->asTextOrThrow();
        if (checksum != expected) {
            throwDatasetError("The source checksum does not match the checksum recorded in the manifest."_el, path);
        }
        el::String text;
        text = el::StringDecoder{bytes}
                   .decode(el::StringEncoding::Utf8, el::StringBomMode::Automatic, el::EncodingMode::Tolerant)
                   .replacedAll("\r\n"_el, "\n"_el)
                   .replacedAll("\r"_el, "\n"_el);
        validateHeader(name, version, text);
        _texts.set(name, text);
    }
}

void UcdDataset::validateHeader(const el::String &name, const el::String &version, const el::String &text) const {
    static constexpr auto cTextExtension = ".txt"_el;
    static const auto cHeaderWhitespace = el::CharSet{U' ', U'\t', U'\r'};
    if (name == "UnicodeData.txt"_el) {
        return;
    }
    if (name == "ReadMe.txt"_el) {
        if (!text.contains(el::StringFormat{"Version {} of the Unicode Standard"_el}.build(version))) {
            throwDatasetError("The UCD ReadMe does not identify the expected Unicode version."_el, sourcePath(name));
        }
        return;
    }
    auto headers = el::StringSplitter{text, U'\n'};
    const auto fileHeader = el::StringFormat{"# {}-{}{}"_el}.build(
        name.slice(el::StringSide::Front, name.length() - cTextExtension.length()), version, cTextExtension);
    const auto alternateHeader = el::StringFormat{"# {}-{}{}"_el}.build(name, version, cTextExtension);
    const auto versionHeader = el::StringFormat{"# Version: {}"_el}.build(version);
    while (!headers.isAtEnd()) {
        const auto line = headers.next().trimmed(cHeaderWhitespace);
        if (!line.isEmpty() && !line.startsWith("#"_el)) {
            break;
        }
        // Fixture and upstream headers both identify a complete version token.
        if (line == fileHeader || line == versionHeader || line == alternateHeader) {
            return;
        }
    }
    throwDatasetError("The source header does not identify the expected Unicode version."_el, sourcePath(name));
}

auto UcdDataset::text(const el::String &name) const -> el::String {
    if (!_texts.contains(name)) {
        throwDatasetError(
            el::StringFormat{"The requested UCD input is not part of the verified dataset: {}."_el}.build(name));
    }
    return _texts.get(name, el::String{});
}

auto UcdDataset::sourcePath(const el::String &name) const -> el::Path {
    return _directory / name;
}

void UcdDataset::throwDatasetError(
    el::String description,
    const el::Path &path,
    const std::exception_ptr &cause,
    const el::CodeLocation location) const {
    auto sourcePath = _directory.toString();
    if (!path.isEmpty()) {
        sourcePath = path.toString();
    }
    throw el::ApplicationError{"Failed to Load UCD Dataset"_el, std::move(description), cause}
        .withSourcePath(std::move(sourcePath))
        .withCodeLocation(location);
}

}
