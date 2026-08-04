// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeNormalizer.hpp"

#include "UnicodeNormalizationData.hpp"

#include "../Char.hpp"

#include <algorithm>
#include <exception>

namespace erbsland::text::impl {

UnicodeNormalizer::UnicodeNormalizer(const NormalizationForm form) noexcept : _form{form} {
}

auto UnicodeNormalizer::usesCompatibilityDecomposition() const noexcept -> bool {
    return _form == NormalizationForm::Nfkc || _form == NormalizationForm::Nfkd;
}

auto UnicodeNormalizer::usesCanonicalComposition() const noexcept -> bool {
    return _form == NormalizationForm::Nfc || _form == NormalizationForm::Nfkc;
}

void UnicodeNormalizer::appendDecomposition(const char32_t codePoint) noexcept {
    const auto syllableIndex = static_cast<uint32_t>(codePoint - cHangulSyllableBase);
    if (syllableIndex < cHangulSyllableCount) {
        if (_decompositionLength + 3U > _decomposition.size()) {
            std::terminate();
        }
        _decomposition[_decompositionLength++] =
            cHangulLeadingBase + static_cast<char32_t>(syllableIndex / cHangulBlockCount);
        _decomposition[_decompositionLength++] =
            cHangulVowelBase + static_cast<char32_t>((syllableIndex % cHangulBlockCount) / cHangulTrailingCount);
        const auto trailingIndex = syllableIndex % cHangulTrailingCount;
        if (trailingIndex != 0U) {
            _decomposition[_decompositionLength++] = cHangulTrailingBase + static_cast<char32_t>(trailingIndex);
        }
        return;
    }

    const auto mapping = unicodeDecompositionFor(codePoint);
    if (!mapping.isPresent() || (mapping.compatibility && !usesCompatibilityDecomposition())) {
        if (_decompositionLength == _decomposition.size()) {
            std::terminate();
        }
        _decomposition[_decompositionLength++] = codePoint;
        return;
    }
    for (auto index = uint8_t{0}; index < mapping.length; ++index) {
        appendDecomposition(mapping.codePoints[index]);
    }
}

auto UnicodeNormalizer::prepare(const char32_t codePoint) -> bool {
    if (_isPrepared || _isFinished) {
        std::terminate();
    }
    _preparedSource = codePoint;
    _decompositionLength = 0U;
    appendDecomposition(codePoint);
    _isPrepared = true;
    if (isEmpty()) {
        return false;
    }

    const auto first = _decomposition[0];
    if (unicodeCombiningClassFor(first) != 0U) {
        return false;
    }
    if (_sequenceOverflow || _sequenceLength == 0U || unicodeCombiningClassFor(_sequence[0]) != 0U) {
        return true;
    }
    if (!usesCanonicalComposition() || _sequenceLength != 1U) {
        return true;
    }
    return compositionFor(_sequence[0], first) == 0;
}

void UnicodeNormalizer::appendPrepared() noexcept {
    if (!_isPrepared || _isFinished) {
        std::terminate();
    }
    const auto isDiscardedOverflowContinuation = _sequenceOverflow && unicodeCombiningClassFor(_decomposition[0]) != 0U;
    if (!isDiscardedOverflowContinuation) {
        if (_sourceLength == _source.size()) {
            std::terminate();
        }
        _source[_sourceLength++] = _preparedSource;
    }
    for (auto index = std::size_t{0}; index < _decompositionLength; ++index) {
        appendDecomposed(_decomposition[index]);
    }
    _isPrepared = false;
}

void UnicodeNormalizer::appendDecomposed(const char32_t codePoint) noexcept {
    const auto combiningClass = unicodeCombiningClassFor(codePoint);
    if (combiningClass == 0U) {
        if (_sequenceOverflow) {
            appendNormalized(Char::replacement().toRawValue());
            _sequenceOverflow = false;
        } else if (_sequenceLength != 0U) {
            if (usesCanonicalComposition() && _sequenceLength == 1U && unicodeCombiningClassFor(_sequence[0]) == 0U) {
                const auto composite = compositionFor(_sequence[0], codePoint);
                if (composite != 0) {
                    _sequence[0] = composite;
                    return;
                }
            }
            finishSequence();
        }
        _sequence[0] = codePoint;
        _sequenceLength = 1U;
        _nonStarterCount = 0U;
        return;
    }

    if (_sequenceOverflow) {
        return;
    }
    if (_nonStarterCount == cMaximumNonStarterCount) {
        _sequenceLength = 0U;
        _nonStarterCount = 0U;
        _sequenceOverflow = true;
        return;
    }
    if (_sequenceLength == _sequence.size()) {
        std::terminate();
    }

    auto position = _sequenceLength;
    while (position > 0U) {
        const auto previousClass = unicodeCombiningClassFor(_sequence[position - 1U]);
        if (previousClass == 0U || previousClass <= combiningClass) {
            break;
        }
        _sequence[position] = _sequence[position - 1U];
        --position;
    }
    _sequence[position] = codePoint;
    ++_sequenceLength;
    ++_nonStarterCount;
}

void UnicodeNormalizer::appendNormalized(const char32_t codePoint) noexcept {
    if (_normalizedLength == _normalized.size()) {
        std::terminate();
    }
    _normalized[_normalizedLength++] = codePoint;
}

void UnicodeNormalizer::finishSequence() noexcept {
    if (_sequenceLength == 0U) {
        return;
    }
    if (!usesCanonicalComposition() || unicodeCombiningClassFor(_sequence[0]) != 0U) {
        for (auto index = std::size_t{0}; index < _sequenceLength; ++index) {
            appendNormalized(_sequence[index]);
        }
    } else {
        const auto starterPosition = _normalizedLength;
        auto starter = _sequence[0];
        appendNormalized(starter);
        auto previousClass = uint8_t{0};
        for (auto index = std::size_t{1}; index < _sequenceLength; ++index) {
            const auto codePoint = _sequence[index];
            const auto combiningClass = unicodeCombiningClassFor(codePoint);
            auto composite = char32_t{0};
            if (previousClass == 0U || previousClass < combiningClass) {
                composite = compositionFor(starter, codePoint);
            }
            if (composite != 0) {
                _normalized[starterPosition] = composite;
                starter = composite;
                continue;
            }
            appendNormalized(codePoint);
            previousClass = combiningClass;
        }
    }
    _sequenceLength = 0U;
    _nonStarterCount = 0U;
}

void UnicodeNormalizer::finish() noexcept {
    if (_isFinished) {
        std::terminate();
    }
    if (_sequenceOverflow) {
        appendNormalized(Char::replacement().toRawValue());
        _sequenceOverflow = false;
    } else {
        finishSequence();
    }
    _isFinished = true;
}

void UnicodeNormalizer::resetWindow() noexcept {
    if (!_isFinished) {
        std::terminate();
    }
    _sourceLength = 0U;
    _normalizedLength = 0U;
    _sequenceLength = 0U;
    _nonStarterCount = 0U;
    _sequenceOverflow = false;
    _isFinished = false;
}

auto UnicodeNormalizer::isUnchanged() const noexcept -> bool {
    return _isFinished && _sourceLength == _normalizedLength &&
        std::equal(_source.begin(), _source.begin() + static_cast<std::ptrdiff_t>(_sourceLength), _normalized.begin());
}

auto UnicodeNormalizer::normalizedCodePoints() const noexcept -> std::span<const char32_t> {
    if (!_isFinished) {
        return {};
    }
    return std::span<const char32_t>{_normalized.data(), _normalizedLength};
}

auto UnicodeNormalizer::composeHangul(const char32_t starter, const char32_t trailing) noexcept -> char32_t {
    const auto leadingIndex = static_cast<uint32_t>(starter - cHangulLeadingBase);
    const auto vowelIndex = static_cast<uint32_t>(trailing - cHangulVowelBase);
    if (leadingIndex < cHangulLeadingCount && vowelIndex < cHangulVowelCount) {
        return cHangulSyllableBase +
            static_cast<char32_t>((leadingIndex * cHangulVowelCount + vowelIndex) * cHangulTrailingCount);
    }

    const auto syllableIndex = static_cast<uint32_t>(starter - cHangulSyllableBase);
    const auto trailingIndex = static_cast<uint32_t>(trailing - cHangulTrailingBase);
    if (syllableIndex < cHangulSyllableCount && syllableIndex % cHangulTrailingCount == 0U && trailingIndex > 0U &&
        trailingIndex < cHangulTrailingCount) {
        return starter + static_cast<char32_t>(trailingIndex);
    }
    return 0;
}

auto UnicodeNormalizer::compositionFor(const char32_t starter, const char32_t trailing) noexcept -> char32_t {
    const auto hangul = composeHangul(starter, trailing);
    return hangul != 0 ? hangul : unicodeCompositionFor(starter, trailing);
}

}
