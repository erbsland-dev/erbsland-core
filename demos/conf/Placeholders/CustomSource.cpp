// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/PlaceholderSource.hpp>

namespace demo {

/// Provide application-owned values for configuration placeholders.
///
/// A placeholder source publishes one or more case-insensitive ELCL names. The parser passes the normalized source name
/// and the decoded, case-preserving parameter to `resolve()`. The returned text replaces the complete placeholder.
class AcademySource final : public el::conf::PlaceholderSource {
public:
    [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"academy"_el}; }

    [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
        if (parameter == "name"_el) {
            return "Academia da Lua"_el;
        }
        if (parameter == "library"_el) {
            return "Biblioteca das Estrelas"_el;
        }
        throw el::conf::ConfError{
            el::conf::ConfErrorCategory::ValueNotFound,
            el::StringFormat{"The academy value '{}' does not exist."_el}.build(parameter)};
    }
};

/// Register a custom source and expand its values while parsing quoted text.
void customSource() {
    auto parser = el::conf::Parser{};
    parser.addPlaceholderSource(std::make_shared<AcademySource>());
    const auto document = parser.parseTextOrThrow(
        "[academy]\n"
        "name: \"${academy:name}\"\n"
        "welcome: \"Bem-vindo à ${academy:library}\"\n"_el);

    el::io::printLine("Academy: "_el, document->getTextOrThrow("academy.name"_el));
    el::io::printLine("Message: "_el, document->getTextOrThrow("academy.welcome"_el));
}

}
