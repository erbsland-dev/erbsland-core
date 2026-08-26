// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpGrammar.hpp"

#include "../../../text/AsciiCategory.hpp"
#include "../../../text/Char.hpp"
#include "../../../text/CharRange.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::network::impl::http_grammar {

using namespace text;
using namespace text::literals;

auto isToken(const String &value) noexcept -> bool {
    return !value.isEmpty() && value.containsOnly(AsciiCategory::HttpToken);
}

auto isFieldValue(const String &value) noexcept -> bool {
    static const auto cFieldValueCharacters = []() -> CharSet {
        auto result = CharSet{Char{U'\t'}};
        result.add(CharRange{0x20U, 0x7eU});
        result.add(CharRange{0x80U, 0x10ffffU});
        result.add(Char::replacement());
        return result;
    }();
    if (value.startsWith(" "_el) || value.startsWith("\t"_el) || value.endsWith(" "_el) || value.endsWith("\t"_el)) {
        return false;
    }
    return value.containsOnly(cFieldValueCharacters);
}

auto isRequestTarget(const String &value) noexcept -> bool {
    static const auto cRequestTargetCharacters = CharSet::fromRange(Char{0x21U}, Char{0x7eU});
    return !value.isEmpty() && value.containsOnly(cRequestTargetCharacters);
}

auto isReasonPhrase(const String &value) noexcept -> bool {
    static const auto cReasonPhraseCharacters = []() -> CharSet {
        auto result = CharSet{Char{U'\t'}};
        result.add(CharRange{0x20U, 0x7eU});
        result.add(CharRange{0x80U, 0x10ffffU});
        result.add(Char::replacement());
        return result;
    }();
    return value.containsOnly(cReasonPhraseCharacters);
}

auto equalTokenCI(const String &left, const String &right) noexcept -> bool {
    return left.compare(right, Char::compareAsciiFolded) == std::strong_ordering::equal;
}

}
