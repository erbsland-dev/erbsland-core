// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BoxFrameSource.hpp"

#include "../frame/TextChecksum.hpp"
#include "../frame/ToolApplication.hpp"
#include "../unicode/UcdReader.hpp"
#include "../unicode/UnicodeSource.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StringDecoder.hpp>
#include <erbsland/StringFormat.hpp>

namespace tool::cterm {

using namespace el::text::literals;

auto BoxFrameSource::sourceIdentifier() -> el::String {
    return "box_frame"_el;
}

auto BoxFrameSource::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Box frame source"_el}, Description{"Character repertoire for terminal frame generation."_el});
    rules.addRule(
        cInputFileConfig,
        RuleType::Text,
        Title{"Box Frame Character Repertoire"_el},
        Description{"An ordered set of Unicode scalars selected for terminal frame generation."_el},
        Default{"tool/data/cterm/box_frame_characters.txt"_el});
    return rules.takeRules();
}

void BoxFrameSource::configure(const el::conf::ValuePtr &configuration) {
    _inputFile = frame::app().files().checked(configuration->valueOrThrow(cInputFileConfig), el::PathType::RegularFile);
}

void BoxFrameSource::initialize() {
    _data.reset();
    const auto bytes = frame::app().files().readData(_inputFile);
    const auto manifest = frame::app().files().readConfiguration(_inputFile.parent() / "manifest.elcl"_el, false);
    const auto digest = frame::calculateTextChecksum(bytes, el::HashAlgorithm::Sha3_256);
    const auto checksum = el::text::base_n::BaseNEncoder{digest, el::text::base_n::BaseNFormat::base16()}.toString();
    const auto key = el::StringFormat{"files.\"{:text:esc=config}\""_el}.build(_inputFile.name());
    if (checksum != manifest->getTextOrThrow(key)) {
        throw el::ApplicationError{
            "Invalid Box Frame Snapshot"_el, "The repertoire file differs from its recorded checksum."_el}
            .withSourcePath(_inputFile.toString());
    }
    const auto text = el::StringDecoder{bytes}
                          .decode(el::StringEncoding::Utf8)
                          .replacedAll("\r\n"_el, "\n"_el)
                          .replacedAll("\r"_el, "\n"_el);
    auto codePoints = std::vector<char32_t>{};
    for (const auto &record : unicode::UcdReader{text, _inputFile.toString()}.parse(el::ItemCount{1}, true)) {
        if (record.first != record.last) {
            throw el::ApplicationError{
                "Invalid Box Frame Snapshot"_el, "The ordered repertoire requires one scalar per line."_el};
        }
        codePoints.push_back(record.first);
    }
    _data = std::make_unique<BoxFrameData>(codePoints, frame::app().source<unicode::UnicodeSource>().unicodeData());
}

auto BoxFrameSource::data() const -> const BoxFrameData & {
    if (!_data) {
        throw el::LogicFailure{"The box frame source is not initialized."_el};
    }
    return *_data;
}

}
