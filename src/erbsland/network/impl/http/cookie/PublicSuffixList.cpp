// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicSuffixList.hpp"

#include "PublicSuffixLookup.hpp"

#include "../../../../text/CharSet.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringSide.hpp"
#include "../../../../unit/ByteLength.hpp"

namespace erbsland::network::impl::public_suffix {

using namespace text::literals;

auto isPublicSuffix(const text::String &host) -> bool {
    if (host.isEmpty()) {
        return false;
    }
    const auto labelCount = host.count("."_el).toSizeT() + 1U;
    return PublicSuffixLookup{}.publicSuffixLabelCount(host) == labelCount;
}

auto registrableDomain(const text::String &host) -> text::String {
    const auto suffixLabels = PublicSuffixLookup{}.publicSuffixLabelCount(host);
    if (suffixLabels == 0U) {
        return {};
    }
    const auto labelSeparator = text::CharSet{U'.'};
    auto split = host.indexAt(text::StringSide::Back);
    for (auto label = std::size_t{}; label < suffixLabels; ++label) {
        split = host.findLastOf(labelSeparator, split);
        if (split.isNoIndex()) {
            return {};
        }
    }
    split = host.findLastOf(labelSeparator, split);
    if (split.isNoIndex()) {
        return host;
    }
    return host.slice(text::StringSide::Back, split + unit::ByteLength::one());
}

}
