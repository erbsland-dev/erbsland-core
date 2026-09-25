// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/text/placeholder/Filter.hpp>
#include <erbsland/text/placeholder/Source.hpp>

namespace demo {

class LiteralSource final : public el::placeholder::Source {
public:
    [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"literal"_el}; }
    [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
        return parameter;
    }
};

/// Transform placeholder values with an application-owned filter.
///
/// A filter publishes one or more names and receives the current text, including changes made by earlier filters in the
/// chain. Its parameter is decoded but otherwise preserved. This filter adds an application label in front of a value.
class AcademyLabelFilter final : public el::placeholder::Filter {
public:
    [[nodiscard]] auto filterNames() const -> el::StringList override { return el::StringList{"academy label"_el}; }

    [[nodiscard]] auto apply(const el::String &, const el::String &parameter, const el::String &value)
        -> el::String override {
        return el::String::fromJoined({"["_el, parameter, "] "_el, value});
    }
};

/// Register a custom filter and apply it after a placeholder source.
void customFilter() {
    auto parser = el::conf::Parser{};
    parser.addPlaceholderSource(std::make_shared<LiteralSource>());
    parser.addPlaceholderFilter(std::make_shared<AcademyLabelFilter>());
    const auto document = parser.parseTextOrThrow(
        "[academy]\n"
        "room: \"${literal:Biblioteca Arcana|academy label:ala norte}\"\n"_el);

    el::io::printLine("Room: "_el, document->getTextOrThrow("academy.room"_el));
}

}
