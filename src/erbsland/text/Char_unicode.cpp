// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Char.hpp"

#include "impl/UnicodeData.hpp"

#include <cstddef>

namespace erbsland::text {

auto Char::applyDelta(const char32_t codePoint, const int32_t delta) noexcept -> Char {
    return Char{static_cast<char32_t>(static_cast<int32_t>(codePoint) + delta)};
}

auto Char::category() const noexcept -> UnicodeCategory {
    if (!isValidUnicode()) {
        return UnicodeCategory::Unassigned;
    }
    return impl::unicodeDataFor(_codePoint).category();
}

auto Char::isControl() const noexcept -> bool {
    if (!isValidUnicode()) {
        return false;
    }
    return category() == UnicodeCategory::Control;
}

auto Char::isControlOrFormat() const noexcept -> bool {
    if (!isValidUnicode()) {
        return false;
    }
    const auto unicodeCategory = category();
    return unicodeCategory == UnicodeCategory::Control || unicodeCategory == UnicodeCategory::Format;
}

auto Char::displayWidth() const noexcept -> int {
    if (!isValidUnicode()) {
        return 0;
    }
    return static_cast<int>(impl::unicodeDisplayWidthFor(_codePoint));
}

auto Char::caseFolded() const noexcept -> Char {
    if (_codePoint >= U'A' && _codePoint <= U'Z') {
        return Char{static_cast<char32_t>(_codePoint + (U'a' - U'A'))};
    }
    if (!isValidUnicode() || _codePoint < 0x80U) {
        return *this;
    }
    const auto &data = impl::unicodeDataFor(_codePoint);
    return applyDelta(_codePoint, impl::unicodeDeltaFor(data).caseFold);
}

auto Char::toLowercase() const noexcept -> Char {
    if (_codePoint >= U'A' && _codePoint <= U'Z') {
        return Char{static_cast<char32_t>(_codePoint + (U'a' - U'A'))};
    }
    if (!isValidUnicode() || _codePoint < 0x80U) {
        return *this;
    }
    const auto &data = impl::unicodeDataFor(_codePoint);
    return applyDelta(_codePoint, impl::unicodeDeltaFor(data).lowercase);
}

auto Char::toUppercase() const noexcept -> Char {
    if (_codePoint >= U'a' && _codePoint <= U'z') {
        return Char{static_cast<char32_t>(_codePoint - (U'a' - U'A'))};
    }
    if (!isValidUnicode() || _codePoint < 0x80U) {
        return *this;
    }
    const auto &data = impl::unicodeDataFor(_codePoint);
    return applyDelta(_codePoint, impl::unicodeDeltaFor(data).uppercase);
}

auto Char::compareAsciiFolded(const Char left, const Char right) noexcept -> std::strong_ordering {
    return left.compareAsciiFolded(right);
}

auto Char::compareCaseFolded(const Char other) const noexcept -> std::strong_ordering {
    return caseFolded() <=> other.caseFolded();
}

auto Char::compareCaseFolded(const Char left, const Char right) noexcept -> std::strong_ordering {
    return left.compareCaseFolded(right);
}

auto Char::compareIdentifier(const Char left, const Char right) noexcept -> std::strong_ordering {
    return left.compareIdentifier(right);
}

}
