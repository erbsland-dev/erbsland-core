// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EmbeddedDocuments.hpp"

#include <erbsland/all.hpp>
#include <erbsland/conf/Parser.hpp>

#include <cstdint>

namespace profiling::conf {

namespace el = erbsland;

using namespace el::text::literals;

constexpr auto cDefaultIterationCount = el::OptionInteger{100};
constexpr auto cFnvOffset = std::uint64_t{1469598103934665603ULL};
constexpr auto cFnvPrime = std::uint64_t{1099511628211ULL};

[[nodiscard]] auto corpusChecksum() noexcept -> std::uint64_t {
    auto result = cFnvOffset;
    for (const auto &document : cEmbeddedDocuments) {
        for (const auto character : document.text) {
            result ^= static_cast<std::uint8_t>(character);
            result *= cFnvPrime;
        }
    }
    return result;
}

[[nodiscard]] auto corpusSize() noexcept -> el::ByteLength {
    auto result = el::ByteLength{};
    for (const auto &document : cEmbeddedDocuments) {
        result += el::ByteLength{document.text.size()};
    }
    return result;
}

[[nodiscard]] auto parseCorpus(
    el::conf::Parser &parser, const el::StringList &documents, const el::OptionInteger iterationCount)
    -> std::uint64_t {
    auto result = cFnvOffset;
    for (auto iteration = el::OptionInteger{}; iteration < iterationCount; ++iteration) {
        for (const auto &text : documents) {
            const auto document = parser.parseTextOrThrow(text);
            result ^= document->size();
            result *= cFnvPrime;
        }
    }
    return result;
}

class ConfParserProfileApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement el::Application
    void initialize() override {
        info().setApplicationName("ELCL Parser Profiling"_el);
        info().setApplicationVersion(el::Version{1, 0, 0});
    }

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpTitle("ELCL Parser Profiling"_el);
        options->setHelpDescription(
            "Repeatedly parses an embedded corpus of complex ELCL documents for performance profiling."_el);
        options->addOption({"--iterations"_el, "iterations"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(cDefaultIterationCount)
            .setValueName("count"_el)
            .setHelpDescription("Number of times to parse the complete corpus after one warm-up pass."_el);
    }

    [[nodiscard]] auto main() -> el::ExitCode override {
        const auto iterationCount = optionValues()->getInteger("iterations"_el);
        if (iterationCount < 1) {
            throw el::ApplicationError{"The iteration count must be a positive integer."_el};
        }

        auto documents = el::StringList{};
        for (const auto &document : cEmbeddedDocuments) {
            documents.append(el::String{document.text});
        }

        auto parser = el::conf::Parser{};
        static_cast<void>(parseCorpus(parser, documents, 1)); // Warm caches before the measured work.
        auto timer = el::ElapsedTimer{};
        const auto resultChecksum = parseCorpus(parser, documents, iterationCount);
        const auto elapsedMilliseconds = timer.elapsed().toNanoseconds().converted<el::Milliseconds>();
        const auto corpusByteLength = corpusSize();
        const auto documentCount = cEmbeddedDocuments.size();

        el::io::printLine(
            "implementation=erbsland-core iterations="_el,
            iterationCount,
            " documents="_el,
            documentCount,
            " parsed-documents="_el,
            iterationCount * static_cast<el::OptionInteger>(documentCount),
            " corpus-bytes="_el,
            corpusByteLength.toRawValue(),
            " parsed-bytes="_el,
            iterationCount * static_cast<el::OptionInteger>(corpusByteLength.toRawValue()),
            " corpus-checksum="_el,
            corpusChecksum(),
            " result-checksum="_el,
            resultChecksum,
            " elapsed-ms="_el,
            elapsedMilliseconds.toRawValue());
        return el::ExitCode::success();
    }
};

auto main(const int argc, char *argv[]) -> int {
    auto application = ConfParserProfileApplication{argc, argv};
    return application.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return profiling::conf::main(argc, argv);
}
