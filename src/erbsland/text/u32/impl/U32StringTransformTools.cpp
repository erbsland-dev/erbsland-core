// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringTransformTools.hpp"

#include "../U32String.hpp"

#include "../../../util/impl/LoopControl.hpp"
#include "../../impl/EscapeFormatter.hpp"
#include "../../impl/SafeStringEscapeTools.hpp"
#include "../../StringBuilder.hpp"

#include <string_view>

namespace erbsland::text::impl {

auto U32StringTransformTools::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    auto result = util::LoopResult::Success;
    const auto completed =
        utf32::forEachDecodedCharacter(_data.dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
            const auto status = function(character);
            if (status == util::LoopStatus::Continue) {
                return true;
            }
            result = util::impl::loopStatusToResult(status);
            return false;
        });
    return completed ? util::LoopResult::Success : result;
}

auto U32StringTransformTools::transformedIfChanged(const TransformCharacterFn function) const
    -> std::optional<U32StringSharedStorage> {

    if (function == nullptr) {
        return std::nullopt;
    }

    const auto data = _data.dataSpan();
    auto position = unit::CpIndex::zero();
    auto result = U32StringSharedStorage{};
    auto appendTools = U32StringAppendTools{result};
    auto changed = false;
    const auto startResult = [&](const std::size_t unchangedEnd) -> void {
        if (changed) {
            return;
        }
        changed = true;
        if (!data.empty()) {
            result.ensureMutableCapacity(data.size());
        }
        if (unchangedEnd > 0U) {
            appendTools.append(U32StringDataView{data, unit::CpRange::fromSizeT(unchangedEnd)});
        }
    };

    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf32::decodeCharOrReplace(data, position);
        const auto mapped = function(character);
        if (mapped.isEndOfData()) {
            startResult(begin);
            return result;
        }
        if (mapped.isNoCodePoint()) {
            startResult(begin);
            continue;
        }
        if (!changed && mapped == character) {
            continue;
        }
        startResult(begin);
        appendTools.append(mapped);
    }
    if (changed) {
        return result;
    }
    return std::nullopt;
}

auto U32StringTransformTools::transformed(const TransformCharacterFn function) const -> U32StringSharedStorage {
    if (auto result = transformedIfChanged(function)) {
        return std::move(*result);
    }
    return U32StringSharedStorage{_data};
}

auto U32StringTransformTools::aligned(
    const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto currentLength = unit::CpLength::fromSizeT(data.size());
    if (length <= currentLength || !fill.isValidUnicode()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }
    const auto padding = length - currentLength;
    auto leftPadding = unit::CpLength::zero();
    auto rightPadding = unit::CpLength::zero();
    if (alignment.isRight()) {
        leftPadding = padding;
    } else if (alignment.isHorizontalCenter()) {
        leftPadding = padding / 2U;
        rightPadding = padding - leftPadding;
    } else {
        rightPadding = padding;
    }
    auto result = U32StringSharedStorage{};
    U32StringAppendTools appendTools{result};
    appendTools.append(fill, leftPadding);
    appendTools.append(_data);
    appendTools.append(fill, rightPadding);
    return result;
}

auto U32StringTransformTools::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U32StringDataView &ellipsis) const
    -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    if (maximumWidth.isInfinite()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }
    if (maximumWidth.isZero()) {
        return {};
    }
    if (unit::CpLength::fromSizeT(data.size()) <= maximumWidth) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }

    auto ellipsisLength = unit::CpLength::fromSizeT(ellipsis.dataSpan().size());
    auto ellipsisView = ellipsis;
    if (ellipsisLength > maximumWidth) {
        ellipsisLength = unit::CpLength::zero();
        ellipsisView = {};
    }
    const auto keepLength = maximumWidth - ellipsisLength;
    auto result = U32StringSharedStorage{};
    auto appendTools = U32StringAppendTools{result};
    switch (mode) {
    case TruncateMode::Begin: {
        const auto suffixStart = unit::CpIndex::fromSizeT(data.size()).retreated(keepLength);
        appendTools.append(ellipsisView);
        appendTools.append(dataView(
            unit::CpRange{suffixStart, unit::CpIndex::fromSizeT(data.size())}.withOrigin(_data.range().index())));
        break;
    }
    case TruncateMode::Middle: {
        const auto prefixLength = keepLength.added(unit::CpLength::one()) / 2U;
        const auto suffixLength = keepLength - prefixLength;
        const auto suffixStart = unit::CpIndex::fromSizeT(data.size()).retreated(suffixLength);
        appendTools.append(
            dataView(unit::CpRange{unit::CpIndex::zero(), prefixLength}.withOrigin(_data.range().index())));
        appendTools.append(ellipsisView);
        appendTools.append(dataView(
            unit::CpRange{suffixStart, unit::CpIndex::fromSizeT(data.size())}.withOrigin(_data.range().index())));
        break;
    }
    case TruncateMode::End:
    default:
        appendTools.append(
            dataView(unit::CpRange{unit::CpIndex::zero(), keepLength}.withOrigin(_data.range().index())));
        appendTools.append(ellipsisView);
        break;
    }
    return result;
}

auto U32StringTransformTools::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept
    -> unit::CpLength {
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return unit::CpLength::fromSizeT(_data.dataSpan().size());
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto length = std::size_t{0U};
    utf32::forEachDecodedCharacter(_data.dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            length += formatter->escapeSize(character, StringKind::U32);
        } else {
            length += character.encodedSize(StringKind::U32);
        }
        return true;
    });
    return unit::CpLength::fromSizeT(length);
}

auto U32StringTransformTools::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U32String {
    const auto data = _data.dataSpan();
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return U32String{std::u32string_view{data.data(), data.size()}};
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto builder = StringBuilder{StringKind::U32};
    utf32::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            formatter->escape(character, builder);
        } else {
            builder.append(character);
        }
        return true;
    });
    return builder.takeU32String();
}

auto U32StringTransformTools::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const
    -> U32String {
    auto scanner = SafeStringEscapeTools{maximumWidth, flags};
    const auto data = _data.dataSpan();
    auto position = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto sourceStart = position.toSizeT();
        const auto character = utf32::decodeCharOrReplace(data, position);
        if (!scanner.add(character, sourceStart)) {
            break;
        }
    }
    scanner.finish(data.size());
    auto builder = StringBuilder{StringKind::U32};
    scanner.appendTo(builder, data.size());
    return builder.takeU32String();
}

auto U32StringTransformTools::dataView(const unit::CpRange range) const -> U32StringDataView {
    return U32StringDataView{_data.data(), range};
}

}
