// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeSource.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/LogicFailure.hpp>

namespace tool::unicode {

using namespace el::text::literals;

auto UnicodeSource::sourceIdentifier() -> el::String {
    return "unicode"_el;
}

auto UnicodeSource::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Unicode sources"_el}, Description{"Pinned Unicode version and source directory used by generators."_el});
    rules.addRule(
        cUcdDirectoryConfig,
        RuleType::Text,
        Title{"Unicode Source Directory"_el},
        Description{"The directory containing the pinned Unicode files and their integrity manifest."_el},
        Default{"tool/data/ucd"_el},
        Matches{R"(\A[^\r\n]+\z)"_el});
    rules.addRule(
        cUnicodeVersionConfig,
        RuleType::Text,
        Title{"Expected Unicode Version"_el},
        Description{"An explicit version pin that must match the source manifest and applicable file headers."_el},
        Matches{R"(\A[0-9]+\.[0-9]+\.[0-9]+\z)"_el});
    return rules.takeRules();
}

void UnicodeSource::configure(const el::conf::ValuePtr &configuration) {
    _directory =
        frame::app().files().checked(configuration->valueOrThrow(cUcdDirectoryConfig), el::PathType::Directory);
    _version = configuration->getTextOrThrow(cUnicodeVersionConfig);
}

void UnicodeSource::initialize() {
    _mapping.reset();
    log()->trace("Loading Unicode sources: "_el, _directory, " ("_el, _version, ")"_el);
    auto names = UcdDataset::filenames();
    names.append("CaseFolding.txt"_el);
    names.append("CompositionExclusions.txt"_el);
    names.append("EastAsianWidth.txt"_el);
    names.append("IdnaMappingTable.txt"_el);
    names.append("ReadMe.txt"_el);
    _dataset = std::make_unique<UcdDataset>(_directory, _version, names);
    _unicodeData = std::make_unique<UnicodeData>(*_dataset);
    _idnaData = std::make_unique<idna::IdnaData>(*_dataset, *_unicodeData);
    _normalizer = std::make_unique<NfcNormalizer>(*_dataset, *_unicodeData);
    _mapping = std::make_unique<IdnaMapping>(*_dataset);
}

void UnicodeSource::requireInitialized() const {
    if (!_mapping) {
        throw el::LogicFailure{"Unicode source initialization has not completed."_el};
    }
}

auto UnicodeSource::dataset() const -> const UcdDataset & {
    requireInitialized();
    return *_dataset;
}

auto UnicodeSource::unicodeData() const -> const UnicodeData & {
    requireInitialized();
    return *_unicodeData;
}

auto UnicodeSource::idnaData() const -> const idna::IdnaData & {
    requireInitialized();
    return *_idnaData;
}

auto UnicodeSource::normalizer() const -> const NfcNormalizer & {
    requireInitialized();
    return *_normalizer;
}

auto UnicodeSource::mapping() const -> const IdnaMapping & {
    requireInitialized();
    return *_mapping;
}

}
