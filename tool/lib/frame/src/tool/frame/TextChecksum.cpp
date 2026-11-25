// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextChecksum.hpp"

#include <erbsland/Hasher.hpp>
#include <erbsland/StringSplitter.hpp>

namespace tool::frame {

using namespace el::text::literals;

auto calculateTextChecksum(const el::ByteBlock &bytes, const el::HashAlgorithm algorithm) -> el::ByteBlock {
    auto hasher = el::Hasher{algorithm};
    const auto data = bytes.span();
    auto start = std::size_t{0};
    for (auto index = std::size_t{0}; index < data.size(); ++index) {
        if (data[index] != '\r') {
            continue;
        }
        hasher.update(data.subspan(start, index - start));
        hasher.update("\n"_el);
        if (index + 1 < data.size() && data[index + 1] == '\n') {
            ++index;
        }
        start = index + 1;
    }
    hasher.update(data.subspan(start));
    return hasher.finalize();
}

auto calculateTextChecksum(const el::String &text, const el::HashAlgorithm algorithm) -> el::ByteBlock {
    auto hasher = el::Hasher{algorithm};
    auto lines = el::StringSplitter{text, U'\n', el::StringSplitMode::KeepSeparator};
    while (!lines.isAtEnd()) {
        const auto line = lines.next();
        hasher.update(line.replacedAll("\r\n"_el, "\n"_el).replacedAll("\r"_el, "\n"_el));
    }
    return hasher.finalize();
}

}
