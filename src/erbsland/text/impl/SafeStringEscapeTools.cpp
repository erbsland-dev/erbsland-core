// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SafeStringEscapeTools.hpp"

#include "../AnyStringBuilder.hpp"
#include "../String.hpp"
#include "../u32/U32String.hpp"
#include "../u32/U32StringConstIterator.hpp"
#include "../u8/U8StringConstIterator.hpp"

#include <algorithm>

namespace erbsland::text::impl {

SafeStringEscapeTools::SafeStringEscapeTools(const unit::CpLength maximumWidth, const SafeStringFlags flags) :
    _maximumWidth{maximumWidth},
    _flags{flags},
    _amount{
        flags.isSet(SafeStringFlag::OnlyAscii) ? EscapeAmount{EscapeAmount::NonAscii}
                                               : EscapeAmount{EscapeAmount::Balanced}},
    _formatter{EscapeFormatter::forFormat(EscapeFormat::Cpp)} {
}

auto SafeStringEscapeTools::add(const Char character, const std::size_t sourceStart) -> bool {
    if (_truncated) {
        return false;
    }
    auto chunk = escapedChunk(character);
    chunk.sourceStart = sourceStart;
    const auto remainingWidth = _maximumWidth.toSizeT() - std::min(_outputLength, _maximumWidth.toSizeT());
    if (chunk.text.size() > remainingWidth) {
        _truncated = true;
        _remainingStart = sourceStart;
        return false;
    }
    _outputLength += chunk.text.size();
    _chunks.emplace_back(std::move(chunk));
    return true;
}

void SafeStringEscapeTools::finish(const std::size_t sourceEnd) noexcept {
    if (!_truncated) {
        _remainingStart = sourceEnd;
    }
}

void SafeStringEscapeTools::appendTo(AnyStringBuilder &builder, const std::size_t sourceLength) const {
    if (_maximumWidth.isZero()) {
        return;
    }

    const auto maximumOutput = _maximumWidth.toSizeT();
    auto suffixText = std::vector<Char>{};
    auto quoteWidth = std::size_t{0U};
    auto bodyBudget = maximumOutput;

    if (_truncated) {
        auto selectedCount = _chunks.size();
        for (auto attempt = 0U; attempt < 4U; ++attempt) {
            const auto remainingStart =
                selectedCount < _chunks.size() ? _chunks[selectedCount].sourceStart : _remainingStart;
            const auto remainingUnits = sourceLength - remainingStart;
            const auto currentSuffixLength = suffixLength(remainingUnits);
            quoteWidth = _flags.isSet(SafeStringFlag::AutoQuotes) ? 2U : 0U;
            if (currentSuffixLength + quoteWidth > maximumOutput) {
                suffixText.clear();
                quoteWidth = 0U;
                bodyBudget = maximumOutput;
                break;
            }
            suffixText = suffix(remainingUnits);
            bodyBudget = maximumOutput - suffixText.size() - quoteWidth;
            const auto newSelectedCount = selectedChunkCount(bodyBudget);
            if (newSelectedCount == selectedCount) {
                break;
            }
            selectedCount = newSelectedCount;
        }
    }

    const auto selectedCount = selectedChunkCount(bodyBudget);
    auto quoteNeeded = _flags.isSet(SafeStringFlag::AutoQuotes) && (!suffixText.empty() || needsQuotes(selectedCount));
    if (quoteNeeded && suffixText.empty() && bodyLength(selectedCount) + 2U > maximumOutput) {
        quoteNeeded = false;
    }

    if (quoteNeeded) {
        builder.append(Char{U'"'});
    }
    for (auto index = std::size_t{0U}; index < selectedCount; ++index) {
        for (const auto character : _chunks[index].text) {
            builder.append(character);
        }
    }
    for (const auto character : suffixText) {
        builder.append(character);
    }
    if (quoteNeeded) {
        builder.append(Char{U'"'});
    }
}

auto SafeStringEscapeTools::escapedChunk(const Char character) const -> Chunk {
    auto result = Chunk{};
    if (_formatter->needsEscape(character, _amount)) {
        auto builder = AnyStringBuilder{StringKind::U32};
        _formatter->escape(character, builder);
        const auto escapedText = builder.takeU32String();
        for (const auto escapedCharacter : escapedText) {
            result.text.emplace_back(escapedCharacter);
        }
        result.quoteNeeded = true;
    } else {
        result.text.emplace_back(character);
        result.quoteNeeded = character.isAsciiWhitespace() || character == U'"';
    }
    return result;
}

auto SafeStringEscapeTools::selectedChunkCount(const std::size_t budget) const noexcept -> std::size_t {
    auto result = std::size_t{0U};
    auto used = std::size_t{0U};
    while (result < _chunks.size() && _chunks[result].text.size() <= budget - std::min(used, budget)) {
        used += _chunks[result].text.size();
        ++result;
    }
    return result;
}

auto SafeStringEscapeTools::needsQuotes(const std::size_t chunkCount) const noexcept -> bool {
    return std::ranges::any_of(
        _chunks.begin(),
        _chunks.begin() + static_cast<std::ptrdiff_t>(chunkCount),
        [](const Chunk &chunk) noexcept -> bool { return chunk.quoteNeeded; });
}

auto SafeStringEscapeTools::bodyLength(const std::size_t chunkCount) const noexcept -> std::size_t {
    auto result = std::size_t{0U};
    for (auto index = std::size_t{0U}; index < chunkCount; ++index) {
        result += _chunks[index].text.size();
    }
    return result;
}

auto SafeStringEscapeTools::decimalLength(std::size_t value) noexcept -> std::size_t {
    auto result = std::size_t{1U};
    while (value >= 10U) {
        value /= 10U;
        ++result;
    }
    return result;
}

auto SafeStringEscapeTools::suffixLength(const std::size_t remainingUnits) noexcept -> std::size_t {
    return std::size_t{12U} + decimalLength(remainingUnits);
}

auto SafeStringEscapeTools::suffix(const std::size_t remainingUnits) -> std::vector<Char> {
    const auto text = String::fromJoined({String{"(... +"}, String::fromInteger(remainingUnits), String{" more)"}});
    auto result = std::vector<Char>{};
    result.reserve(text.length().toSizeT());
    for (const auto character : text) {
        result.emplace_back(character);
    }
    return result;
}

}
