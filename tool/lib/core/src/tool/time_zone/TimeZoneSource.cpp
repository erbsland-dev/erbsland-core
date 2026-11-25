// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZoneSource.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/StringDecoder.hpp>
#include <erbsland/StringFormat.hpp>
#include <tool/frame/TextChecksum.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::time_zone {

using namespace el::text::literals;

auto TimeZoneSource::sourceIdentifier() -> el::String {
    return "time_zone"_el;
}

auto TimeZoneSource::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Time zone sources"_el}, Description{"Pinned time zone source snapshot used by generators."_el});
    rules.addRule(
        cInputDirectoryConfig,
        RuleType::Text,
        Title{"Time-Zone Snapshot Directory"_el},
        Description{"The directory containing tzdb, CLDR, and their integrity manifest."_el},
        Default{"tool/data/time_zone"_el});
    return rules.takeRules();
}

void TimeZoneSource::configure(const el::conf::ValuePtr &configuration) {
    _directory =
        frame::app().files().checked(configuration->valueOrThrow(cInputDirectoryConfig), el::PathType::Directory);
}

void TimeZoneSource::initialize() {
    _texts.clear();
    const auto manifest = frame::app().files().readConfiguration(_directory / "manifest.elcl"_el, false);
    const auto names = el::StringList{
        cVersionFile,
        "tzdb/africa"_el,
        "tzdb/antarctica"_el,
        "tzdb/asia"_el,
        "tzdb/australasia"_el,
        "tzdb/europe"_el,
        "tzdb/northamerica"_el,
        "tzdb/southamerica"_el,
        "tzdb/etcetera"_el,
        "tzdb/backward"_el,
        "windowsZones.xml"_el};
    const auto keyFormat = el::StringFormat{"files.\"{:text:esc=config}\""_el};
    for (const auto &name : names) {
        const auto path = _directory / name;
        const auto bytes = frame::app().files().readData(path);
        const auto digest = frame::calculateTextChecksum(bytes, el::HashAlgorithm::Sha3_256);
        const auto checksum =
            el::text::base_n::BaseNEncoder{digest, el::text::base_n::BaseNFormat::base16()}.toString();
        const auto key = keyFormat.build(name);
        if (checksum != manifest->getTextOrThrow(key)) {
            throw el::ApplicationError{
                "Invalid Time-Zone Snapshot"_el, "A source file differs from its recorded checksum."_el}
                .withSourcePath(path.toString());
        }
        _texts.set(
            name,
            el::StringDecoder{bytes}.decode(
                el::StringEncoding::Utf8, el::StringBomMode::Automatic, el::EncodingMode::Tolerant));
    }
    _version = text(cVersionFile).trimmed();
    if (_version != manifest->getTextOrThrow("dataset.tzdb_version"_el) ||
        _version.characterLength() != el::CpCount{5}) {
        throw el::ApplicationError{
            "Invalid Time-Zone Snapshot"_el, "The tzdb version differs from its manifest or is malformed."_el};
    }
    if (manifest->getTextOrThrow("dataset.cldr_version"_el) != "48"_el) {
        throw el::ApplicationError{"Invalid Time-Zone Snapshot"_el, "Unexpected CLDR release pin."_el};
    }
}

auto TimeZoneSource::text(const el::String &name) const -> el::String {
    if (!_texts.contains(name)) {
        throw el::ApplicationError{"Time-Zone Source Is Not Ready"_el, "Unknown or uninitialized source file."_el};
    }
    return _texts.get(name, el::String{});
}

}
