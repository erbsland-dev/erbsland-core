// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/text/placeholder/Filter.hpp>
#include <erbsland/text/placeholder/ReplacerError.hpp>
#include <erbsland/text/placeholder/Source.hpp>

namespace demo {

/// Create source and filter errors that the parser can enrich with configuration context.
///
/// Providers should report expected lookup and transformation failures as `ReplacerError`. Choose the category that
/// best describes the problem and keep the description useful without exposing protected values.
class RequiredAcademySource final : public el::placeholder::Source {
public:
    [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"academy"_el}; }
    [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
        throw el::placeholder::ReplacerError{
            el::placeholder::ReplacerErrorCategory::Access,
            el::StringFormat{"The academy registry entry '{}' is unavailable."_el}.build(parameter)};
    }
};

class ErrorLiteralSource final : public el::placeholder::Source {
public:
    [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"literal"_el}; }
    [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
        return parameter;
    }
};

class SafeRuneFilter final : public el::placeholder::Filter {
public:
    [[nodiscard]] auto filterNames() const -> el::StringList override { return el::StringList{"safe rune"_el}; }
    [[nodiscard]] auto apply(const el::String &, const el::String &, const el::String &) -> el::String override {
        throw el::placeholder::ReplacerError{
            el::placeholder::ReplacerErrorCategory::Validation,
            "The academy title contains a rune that is not permitted here."_el};
    }
};

/// Report a source failure at the configuration value that requested it.
///
/// Sources and filters can throw `ReplacerError` with a meaningful category and description. The parser adds the
/// placeholder location, the target value's name path, and an available source excerpt before the error reaches the
/// application.
void sourceError() {
    auto parser = el::conf::Parser{};
    parser.addPlaceholderSource(std::make_shared<RequiredAcademySource>());
    const auto document = parser.parseTextOrThrow(
        "[academy]\n"
        "name: \"${academy:secret archive}\"\n"_el);
    el::io::printLine("Unexpected value: "_el, document->getTextOrThrow("academy.name"_el));
}

/// Report a filter failure with the same configuration context.
void filterError() {
    auto parser = el::conf::Parser{};
    parser.addPlaceholderSource(std::make_shared<ErrorLiteralSource>());
    parser.addPlaceholderFilter(std::make_shared<SafeRuneFilter>());
    const auto document = parser.parseTextOrThrow(
        "[academy]\n"
        "title: \"${literal:Grimório Antigo|safe rune}\"\n"_el);
    el::io::printLine("Unexpected value: "_el, document->getTextOrThrow("academy.title"_el));
}

}
