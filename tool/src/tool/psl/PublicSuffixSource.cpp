// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicSuffixSource.hpp"

#include "../frame/TextChecksum.hpp"
#include "../frame/ToolApplication.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/CodeLocation.hpp>
#include <erbsland/conf/Name.hpp>
#include <erbsland/conf/NamePath.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/EncodingError.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringDecoder.hpp>
#include <erbsland/StringSplitter.hpp>

#include <exception>

namespace tool::psl {

using namespace el::text::literals;

auto PublicSuffixSource::sourceIdentifier() -> el::String {
    return "public_suffix"_el;
}

auto PublicSuffixSource::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Public suffix source"_el}, Description{"Pinned public suffix snapshot used by generators."_el});
    rules.addRule(
        cInputFileConfig,
        RuleType::Text,
        Title{"Public Suffix Snapshot"_el},
        Description{
            "The pinned PSL file with a sibling manifest recording its version and line-ending-independent hash."_el},
        Default{"tool/data/public_suffix/public_suffix_list.dat"_el},
        Matches{R"(\A[^\r\n]+\z)"_el});
    return rules.takeRules();
}

void PublicSuffixSource::configure(const el::conf::ValuePtr &configuration) {
    _inputFile = frame::app().files().checked(configuration->valueOrThrow(cInputFileConfig), el::PathType::RegularFile);
}

void PublicSuffixSource::initialize() {
    _initialized = false;
    log()->trace("Loading Public Suffix List source: "_el, _inputFile);
    const auto manifest = frame::app().files().readConfiguration(_inputFile.parent() / "manifest.elcl"_el, false);
    const auto bytes = frame::app().files().readData(_inputFile);
    const auto digest = frame::calculateTextChecksum(bytes, el::HashAlgorithm::Sha3_256);
    const auto hash = el::text::base_n::BaseNEncoder{digest, el::text::base_n::BaseNFormat::base16()}.toString();
    const auto namePath = el::StringFormat("files.\"{:text:esc=config}\"").build(_inputFile.name());
    const auto expected = manifest->getTextOrThrow(namePath);
    if (hash != expected) {
        throw el::ApplicationError{
            "PSL Integrity Check Failed"_el, "The normalized snapshot text differs from the manifest hash."_el}
            .withSourcePath(_inputFile.toString());
    }
    _text = el::StringDecoder{bytes}
                .decode(el::StringEncoding::Utf8, el::StringBomMode::Automatic, el::EncodingMode::Tolerant)
                .replacedAll("\r\n"_el, "\n"_el)
                .replacedAll("\r"_el, "\n"_el);
    _version = manifest->getTextOrThrow("snapshot.version"_el);
    validateVersion(_version);
    _initialized = true;
}

void PublicSuffixSource::validateVersion(const el::String &expected) const {
    static const auto pattern = el::re::RegEx::compile(R"([A-Za-z0-9_.\-]+)"_el);
    static constexpr auto versionLabel = "// VERSION: "_el;
    if (!pattern->fullMatch(expected)) {
        throwSnapshotError("Invalid snapshot version syntax in the manifest."_el);
    }
    auto lines = el::StringSplitter{_text, U'\n'};
    auto found = false;
    lines.forEach([&](const el::String &line, const el::ItemIndex index) -> el::LoopStatus {
        auto trimmedLine = line.trimmed();
        if (line.startsWith(versionLabel)) {
            if (found || line.slice(el::StringSide::Back, el::ByteIndex::end(versionLabel.length())) != expected) {
                throwSnapshotError(
                    "The PSL snapshot version differs from its manifest or is repeated."_el,
                    el::LineIndex::fromSizeT(index.toSizeT()));
            }
            found = true;
        }
        return el::LoopStatus::Continue;
    });
    if (!found) {
        throwSnapshotError("The PSL snapshot has no version header."_el);
    }
}

void PublicSuffixSource::requireInitialized() const {
    if (!_initialized) {
        throw el::LogicFailure{"Public Suffix source initialization has not completed."_el};
    }
}

auto PublicSuffixSource::text() const -> const el::String & {
    requireInitialized();
    return _text;
}

auto PublicSuffixSource::version() const -> const el::String & {
    requireInitialized();
    return _version;
}

void PublicSuffixSource::throwSnapshotError(const el::String &description, const el::LineIndex line) const {
    throw el::ApplicationError{"Invalid PSL Snapshot"_el, description}
        .withSourcePath(_inputFile.toString())
        .withCodeLocation(el::CodeLocation{line});
}

}
