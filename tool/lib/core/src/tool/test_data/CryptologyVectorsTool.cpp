// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CryptologyVectorsTool.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/base_n/BaseNDecoder.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringTransformations.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::test_data {

using namespace el::text::literals;

auto CryptologyVectorsTool::identifier() const -> el::String {
    return "import_cryptology_vectors"_el;
}

auto CryptologyVectorsTool::title() const -> el::String {
    return "Import cryptology test vectors"_el;
}

auto CryptologyVectorsTool::description() const -> el::String {
    return "Download and verify pinned cryptology sources, then prepare response fixtures and their manifest. "
           "Use --source-directory for offline importing."_el;
}

auto CryptologyVectorsTool::defaultDirectory() const -> el::String {
    return "test/unittest/data/cryptology"_el;
}

auto CryptologyVectorsTool::optionModule() const -> el::OptionModulePtr {
    auto module = FixtureTool::optionModule();
    module->addOption({"--source-directory"_el, "source-directory"_el})
        .setType(el::OptionType::Text)
        .setValueName("directory"_el)
        .setHelpTitle("Read sources offline"_el)
        .setHelpDescription("Read only from this project-contained directory; never download missing sources."_el);
    auto choices = el::OptionChoices::create();
    choices->addChoice("all"_el).setHelpTitle("Import all supported families."_el);
    auto families = el::StringList{};
    for (const auto &source : impl::VectorSource::all()) {
        if (!families.contains(source.family)) {
            families.append(source.family);
            choices->addChoice(source.family).setHelpTitle(el::join("Import "_el, source.family, " vectors."_el));
        }
    }
    module->addOption({"--family"_el, "family"_el})
        .setChoices(choices)
        .setValueName("family"_el)
        .setHelpTitle("Select a vector family"_el)
        .setHelpDescription(
            "Import only this family's sources and preserve all unselected fixtures; defaults to all."_el);
    return module;
}

auto CryptologyVectorsTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using namespace el::conf::vr::builder;
    using el::conf::vr::RuleType;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Cryptology vector import"_el},
        Description{"Source, selection and output policies for cryptology vectors."_el});
    rules.addRule(
        cOutputDirectoryValue,
        RuleType::Text,
        Title{"Fixture Output Directory"_el},
        Description{"Project-contained directory receiving response fixtures and provenance."_el},
        Default{defaultDirectory()},
        Minimum{1});
    rules.addRule(
        cMaximumSourceBytesValue,
        RuleType::Integer,
        Title{"Source Size Limit"_el},
        Description{"Maximum bytes in each downloaded or offline source, including the large x509-limbo snapshot."_el},
        Default{134217728},
        Minimum{1},
        Maximum{268435456});
    rules.addRule(
        cDownloadTimeoutSecondsValue,
        RuleType::Integer,
        Title{"Download Deadline"_el},
        Description{"Overall deadline in seconds for each source download, including redirects."_el},
        Default{300},
        Minimum{1},
        Maximum{3600});
    rules.addRule(
        "sources"_el,
        RuleType::Section,
        Title{"Pinned Sources"_el},
        Description{"Locations, content digests and provenance of all supported upstream inputs."_el});
    for (const auto &source : impl::VectorSource::all()) {
        const auto prefix = el::join("sources."_el, source.id);
        rules.addRule(
            prefix,
            RuleType::Section,
            Title{source.filename},
            Description{"Pinned upstream source and attribution."_el});
        rules.addRule(
            el::join(prefix, ".url"_el),
            RuleType::Text,
            Title{"Source URL"_el},
            Description{"Absolute HTTP or HTTPS source URL without credentials or a fragment."_el},
            Minimum{1});
        rules.addRule(
            el::join(prefix, ".sha256"_el),
            RuleType::Text,
            Title{"Source SHA-256"_el},
            Description{"Hex digest of binary ZIP bytes, or text with CRLF and CR normalized to LF."_el},
            Matches{R"(\A[0-9a-fA-F]{64}\z)"_el});
        rules.addRule(
            el::join(prefix, ".revision"_el),
            RuleType::Text,
            Title{"Upstream Revision"_el},
            Description{"Pinned upstream commit, or empty for checksum-pinned archives."_el},
            Matches{R"(\A(?:[0-9a-f]{40})?\z)"_el});
        rules.addRule(
            el::join(prefix, ".attribution"_el),
            RuleType::Text,
            Title{"Source Attribution"_el},
            Description{"Upstream project and licensing information retained in generated provenance."_el},
            Matches{R"(\A[^\r\n]+\z)"_el});
    }
    return rules.takeRules();
}

void CryptologyVectorsTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    FixtureTool::configure(configuration, values);
    _maximumBytes = el::ByteCount::fromIntegerOrThrow(configuration->getIntegerOrThrow(cMaximumSourceBytesValue));
    _timeout = el::TimeDelta::seconds(configuration->getIntegerOrThrow(cDownloadTimeoutSecondsValue));
    _family = values->getText("family"_el, "all"_el);
    _sourceDirectory.reset();
    const auto option = values->value("source-directory"_el);
    if (option) {
        _sourceDirectory = frame::app().files().checked(option);
        if (!_sourceDirectory->info().isDirectory()) {
            throw el::OptionError{
                "Invalid Offline Source Directory"_el, "The source path must be an existing directory."_el, option};
        }
    }
    configureSources(configuration);
    auto outputs = el::StringList{"manifest.txt"_el, "README.rst"_el};
    for (const auto &source : _sources) {
        if (!selected(source.descriptor)) {
            continue;
        }
        outputs.append(source.descriptor.outputs);
        if (_sourceDirectory) {
            try {
                const auto path = frame::app().files().checked(*_sourceDirectory / source.descriptor.filename);
                if (!path.info().isRegularFile()) {
                    throw el::ApplicationError{
                        "Missing Offline Source"_el,
                        el::join("Expected a regular file: "_el, source.descriptor.filename)};
                }
                if (path.info().fileSize() > _maximumBytes) {
                    throw el::ApplicationError{
                        "Offline Source Too Large"_el,
                        el::join("Source exceeds maximum_source_bytes: "_el, source.descriptor.filename)};
                }
            } catch (const el::ApplicationError &error) {
                throw el::OptionError{
                    "Invalid Offline Source Directory"_el, error.description(), option, std::current_exception()};
            }
        }
    }
    validateOutputs(outputs);
}

void CryptologyVectorsTool::configureSources(const el::conf::ValuePtr &configuration) {
    _sources.clear();
    for (const auto &descriptor : impl::VectorSource::all()) {
        const auto section = configuration->valueOrThrow(el::join("sources."_el, descriptor.id));
        const auto urlValue = section->valueOrThrow("url"_el);
        const auto url = el::Url::fromString(urlValue->asTextOrThrow());
        if (!url.isValid() || (url.scheme() != el::UrlScheme::Http && url.scheme() != el::UrlScheme::Https) ||
            !url.username().isEmpty() || !url.password().isEmpty() || url.authorityText().contains("@"_el) ||
            url.hasFragment()) {
            throw el::conf::ConfError{
                urlValue,
                "Invalid Source URL"_el,
                "Expected an absolute HTTP or HTTPS URL without credentials or a fragment."_el};
        }
        auto format = el::base_n::BaseNFormat::base16();
        format.setWhitespace(el::CharSet{});
        const auto checksum =
            el::base_n::BaseNDecoder{el::toAsciiUppercase(section->getTextOrThrow("sha256"_el)), format}
                .toDataOrThrow();
        _sources.append(Source{descriptor, url, checksum, section->getTextOrThrow("revision"_el), section});
    }
}

auto CryptologyVectorsTool::selected(const impl::VectorSource &source) const -> bool {
    return _family == "all"_el || source.family == _family;
}

}
