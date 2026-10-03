// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TypeInWrongUnitRule.hpp"

#include "../impl/RuleSupport.hpp"
#include "../SourceIndex.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto TypeInWrongUnitRule::create() -> RulePtr {
    return std::make_unique<TypeInWrongUnitRule>();
}

auto TypeInWrongUnitRule::identifier() const -> el::String {
    return "type_in_wrong_unit"_el;
}

auto TypeInWrongUnitRule::title() const -> el::String {
    return "Type and filename agreement"_el;
}

auto TypeInWrongUnitRule::description() const -> el::String {
    return "Detect primary types declared in units with unrelated filenames."_el;
}

auto TypeInWrongUnitRule::severity() const -> Severity {
    return Severity::High;
}

void TypeInWrongUnitRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    for (const auto &block : source.blocks()) {
        if ((block.kind != SourceBlock::Kind::Class && block.kind != SourceBlock::Kind::Struct) ||
            !impl::namespaceType(source, block)) {
            continue;
        }
        const auto start = source.typeStart(block);
        const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
        const auto nameStart = header.find(block.name);
        if (!nameStart.isNoIndex()) {
            const auto suffix = header.slice(el::StringSide::Back, nameStart + block.name.length()).trimmed();
            if (!suffix.isEmpty() && !suffix.startsWith(":"_el) && !suffix.startsWith("final"_el) &&
                !suffix.startsWith("requires"_el) && !suffix.startsWith("[["_el) && !suffix.startsWith("<"_el)) {
                continue;
            }
        }
        if (impl::standardExtension(source, block.headerStart, header)) {
            continue;
        }
        if (source.path().suffix() == ".cpp"_el) {
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
