// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringTransformTools.hpp"

#include "U32Writer.hpp"

#include "../U32StringEditor.hpp"

#include "../../../util/impl/LoopControl.hpp"
#include "../../AnyStringBuilder.hpp"
#include "../../impl/EscapeFormatter.hpp"
#include "../../impl/SafeStringEscapeTools.hpp"

#include <cstring>
#include <span>
#include <string_view>

namespace erbsland::text::impl {

using unit::CpIndex;
using unit::CpLength;
using unit::CpRange;

auto U32StringTransformTools::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    auto result = util::LoopResult::Success;
    const auto completed =
        utf32::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
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
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf32::decodeCharOrReplace(data, position);
        const auto mapped = function(character);
        if (mapped.isEndOfData()) {
            return U32StringSharedStorage::fromCodeUnits(data.first(begin));
        }
        if (mapped == character) {
            continue;
        }

        auto reservedSize = U32StringSharedStorage::checkedAddSize(
            begin, static_cast<std::size_t>(mapped.isValidUnicode()), "Transformed string exceeds size bounds");
        auto sizingPosition = position;
        while (sizingPosition.toSizeT() < data.size()) {
            const auto sizingCharacter = utf32::decodeCharOrReplace(data, sizingPosition);
            const auto sizingMapped = function(sizingCharacter);
            if (sizingMapped.isEndOfData()) {
                break;
            }
            reservedSize = U32StringSharedStorage::checkedAddSize(
                reservedSize,
                static_cast<std::size_t>(sizingMapped.isValidUnicode()),
                "Transformed string exceeds size bounds");
        }

        auto result = U32StringSharedStorage{};
        if (reservedSize > 0U) {
            result.ensureMutableCapacity(reservedSize);
        }
        if (begin > 0U) {
            std::memcpy(result.dataForWrite(), data.data(), begin * sizeof(char32_t));
        }

        auto writeSize = begin;
        auto transformPosition = CpIndex::fromSizeT(begin);
        while (transformPosition.toSizeT() < data.size()) {
            const auto transformCharacter = utf32::decodeCharOrReplace(data, transformPosition);
            const auto transformMapped = function(transformCharacter);
            if (transformMapped.isEndOfData()) {
                break;
            }
            if (!transformMapped.isValidUnicode()) {
                continue;
            }
            const auto requiredSize =
                U32StringSharedStorage::checkedAddSize(writeSize, 1U, "Transformed string exceeds size bounds");
            if (requiredSize > result.capacity().toSizeT()) {
                result.ensureMutableCapacity(requiredSize);
            }
            U32Writer{std::span<char32_t>{result.dataForWrite() + writeSize, 1U}}.write(transformMapped);
            writeSize = requiredSize;
        }
        result.resize(writeSize);
        return result;
    }
    return std::nullopt;
}

auto U32StringTransformTools::aligned(const CpLength length, const geometry::Alignment alignment, const Char fill) const
    -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto currentLength = CpLength::fromSizeT(data.size());
    if (length <= currentLength || !fill.isValidUnicode()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }
    const auto padding = length - currentLength;
    auto leftPadding = CpLength::zero();
    auto rightPadding = CpLength::zero();
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
    const CpLength maximumWidth, const TruncateMode mode, const U32StringDataView &ellipsis) const
    -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    if (maximumWidth.isInfinite()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }
    if (maximumWidth.isZero()) {
        return {};
    }
    if (CpLength::fromSizeT(data.size()) <= maximumWidth) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }

    auto ellipsisLength = CpLength::fromSizeT(ellipsis.dataSpan().size());
    auto ellipsisView = ellipsis;
    if (ellipsisLength > maximumWidth) {
        ellipsisLength = CpLength::zero();
        ellipsisView = {};
    }
    const auto keepLength = maximumWidth - ellipsisLength;
    auto result = U32StringSharedStorage{};
    auto appendTools = U32StringAppendTools{result};
    switch (mode) {
    case TruncateMode::Begin: {
        const auto suffixStart = CpIndex::fromSizeT(data.size()).retreated(keepLength);
        appendTools.append(ellipsisView);
        appendTools.append(
            dataView(CpRange{suffixStart, CpIndex::fromSizeT(data.size())}.withOrigin(_data.range().index())));
        break;
    }
    case TruncateMode::Middle: {
        const auto prefixLength = keepLength.added(CpLength::one()) / 2U;
        const auto suffixLength = keepLength - prefixLength;
        const auto suffixStart = CpIndex::fromSizeT(data.size()).retreated(suffixLength);
        appendTools.append(dataView(CpRange{CpIndex::zero(), prefixLength}.withOrigin(_data.range().index())));
        appendTools.append(ellipsisView);
        appendTools.append(
            dataView(CpRange{suffixStart, CpIndex::fromSizeT(data.size())}.withOrigin(_data.range().index())));
        break;
    }
    case TruncateMode::End:
    default:
        appendTools.append(dataView(CpRange{CpIndex::zero(), keepLength}.withOrigin(_data.range().index())));
        appendTools.append(ellipsisView);
        break;
    }
    return result;
}

auto U32StringTransformTools::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept
    -> CpLength {
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return CpLength::fromSizeT(_data.dataSpan().size());
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto length = std::size_t{0U};
    utf32::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            length += formatter->escapeSize(character, StringKind::U32);
        } else {
            length += character.encodedSize(StringKind::U32);
        }
        return true;
    });
    return CpLength::fromSizeT(length);
}

auto U32StringTransformTools::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U32StringEditor {
    const auto data = _data.dataSpan();
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return U32StringEditor{std::u32string_view{data.data(), data.size()}};
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto builder = AnyStringBuilder{StringKind::U32};
    utf32::forEachDecodedCharacter(data, EncodingMode::Tolerant, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            formatter->escape(character, builder);
        } else {
            builder.append(character);
        }
        return true;
    });
    return builder.takeU32StringEditor();
}

auto U32StringTransformTools::toSafeString(const CpLength maximumWidth, const SafeStringFlags flags) const
    -> U32StringEditor {
    auto scanner = SafeStringEscapeTools{maximumWidth, flags};
    const auto data = _data.dataSpan();
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto sourceStart = position.toSizeT();
        const auto character = utf32::decodeCharOrReplace(data, position);
        if (!scanner.add(character, sourceStart)) {
            break;
        }
    }
    scanner.finish(data.size());
    auto builder = AnyStringBuilder{StringKind::U32};
    scanner.appendTo(builder, data.size());
    return builder.takeU32StringEditor();
}

auto U32StringTransformTools::dataView(const CpRange range) const -> U32StringDataView {
    return U32StringDataView{_data.data(), range};
}

}
