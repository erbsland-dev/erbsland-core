// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OversizedFileRule.hpp"

#include "../SourceIndex.hpp"

#include <erbsland/Set.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto OversizedFileRule::create() -> RulePtr {
    return std::make_unique<OversizedFileRule>();
}

auto OversizedFileRule::identifier() const -> el::String {
    return "oversized_file"_el;
}

auto OversizedFileRule::title() const -> el::String {
    return "Source file size"_el;
}

auto OversizedFileRule::description() const -> el::String {
    return "Detect source files exceeding the project line limit."_el;
}

auto OversizedFileRule::severity() const -> Severity {
    return Severity::High;
}

void OversizedFileRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (source.isGenerated()) {
        return;
    }
    auto ignored = el::Set<el::LineIndex>{};
    if (source.path().suffix() == ".hpp"_el) {
        for (const auto index : source.comments()) {
            const auto &comment = source.tokens().getRef(index);
            const auto lineIndex = source.lineIndex(comment.position());
            if (comment.text().startsWith("///"_el) &&
                source.slice(el::ByteRange{source.lineStart(lineIndex), comment.position()}).trimmed().isEmpty()) {
                ignored.insert(lineIndex);
            }
        }
    }
    auto counted = el::LineCount{};
    for (auto lineIndex = el::LineIndex::zero(); lineIndex.isWithin(source.lineCount()); ++lineIndex) {
        if (!ignored.contains(lineIndex) && ++counted > el::LineCount{500}) {
            candidates.append(el::ByteRange{source.lineStart(lineIndex), source.lineEnd(lineIndex)});
            return;
        }
    }
}

}
