// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TypeInWrongUnitRule.hpp"

#include "../impl/RuleSupport.hpp"
#include "../SourceIndex.hpp"

#include <erbsland/RegEx.hpp>

#include <algorithm>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto TypeInWrongUnitRule::create() -> RulePtr {
    return std::make_unique<TypeInWrongUnitRule>();
}

auto TypeInWrongUnitRule::identifier() const -> el::String {
    return "type_in_wrong_unit"_el;
}

auto TypeInWrongUnitRule::title() const -> el::String {
    return "Type placement"_el;
}

auto TypeInWrongUnitRule::description() const -> el::String {
    return "Detect misplaced types and enforce filename agreement for unit-test suites."_el;
}

auto TypeInWrongUnitRule::severity() const -> Severity {
    return Severity::High;
}

auto TypeInWrongUnitRule::isTestSuite(const el::String &header) -> bool {
    static const auto cSuite = el::RegEx::compile(
        R"(\b(?:class|struct)\s+\w+Test(?:\s+final)?\s*:\s*(?:(?:public|protected|private)\s+)?)"
        R"((?:UNITTEST_SUBCLASS\s*\(\s*(?:\w+\s*::\s*)*\w+\s*\))"
        R"(|(?:(?:::)?\s*(?:el|erbsland)\s*::\s*(?:unittest\s*::\s*)?)?UnitTest))"
        R"(\s*(?:,\s*[^{}]+)?\s*\z)"_el);
    return static_cast<bool>(cSuite->findFirst(header));
}

void TypeInWrongUnitRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    const auto isImplementation = source.path().suffix() == ".cpp"_el;
    const auto unit = source.path().stem();
    const auto isTestUnit = isImplementation && unit.endsWith("Test"_el) &&
        std::any_of(source.blocks().begin(), source.blocks().end(), [&](const SourceBlock &block) -> bool {
            return block.name == unit &&
                (block.kind == SourceBlock::Kind::Class || block.kind == SourceBlock::Kind::Struct) &&
                impl::namespaceType(source, block) &&
                isTestSuite(source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true));
        });
    for (const auto &block : source.blocks()) {
        if ((block.kind != SourceBlock::Kind::Class && block.kind != SourceBlock::Kind::Struct) ||
            !impl::namespaceType(source, block)) {
            continue;
        }
        const auto start = source.typeStart(block);
        const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
        const auto declaration = source.slice(el::ByteRange{start, block.openingBrace()}, true);
        const auto nameStart = declaration.find(block.name);
        auto suffix = el::String{};
        if (!nameStart.isNoIndex()) {
            suffix = declaration.slice(el::StringSide::Back, nameStart + block.name.length()).trimmed();
            if (!suffix.isEmpty() && !suffix.startsWith(":"_el) && !suffix.startsWith("final"_el) &&
                !suffix.startsWith("requires"_el) && !suffix.startsWith("[["_el) && !suffix.startsWith("<"_el)) {
                continue;
            }
        }
        if (impl::standardExtension(source, block.headerStart, header)) {
            continue;
        }
        if (isImplementation) {
            if (isTestUnit && !suffix.startsWith("::"_el)) {
                const auto isMisplacedSuite =
                    block.name != unit && block.name.endsWith("Test"_el) && isTestSuite(header);
                if (!isMisplacedSuite) {
                    continue;
                }
            }
            candidates.append(el::ByteRange{start, source.statementEnd(block.braces.endIndex())});
            continue;
        }
        const auto name = impl::namespaceName(source, block);
        if (!impl::matchesSourceDirectory(source, name)) {
            candidates.append(el::ByteRange{start, block.openingBrace().incremented()});
        }
    }
}

}
