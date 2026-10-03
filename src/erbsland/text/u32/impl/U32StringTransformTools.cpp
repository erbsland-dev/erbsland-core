// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringTransformTools.hpp"

#include "U32Writer.hpp"

#include "../U32String.hpp"
#include "../U32StringEditor.hpp"

#include "../../../util/impl/LoopControl.hpp"
#include "../../AnyStringBuilder.hpp"
#include "../../impl/escape/EscapeFormatter.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

using unit::CpCount;
using unit::CpIndex;
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

        auto reservedSize =
            U32StringSharedStorage::checkedAddSize(begin, static_cast<std::size_t>(mapped.isValidUnicode()));
        auto sizingPosition = position;
        while (sizingPosition.toSizeT() < data.size()) {
            const auto sizingCharacter = utf32::decodeCharOrReplace(data, sizingPosition);
            const auto sizingMapped = function(sizingCharacter);
            if (sizingMapped.isEndOfData()) {
                break;
            }
            reservedSize = U32StringSharedStorage::checkedAddSize(
                reservedSize, static_cast<std::size_t>(sizingMapped.isValidUnicode()));
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
            const auto requiredSize = U32StringSharedStorage::checkedAddSize(writeSize, 1U);
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

auto U32StringTransformTools::alignedIfChanged(
    const CpCount length, const geometry::Alignment alignment, const Char fill) const
    -> std::optional<U32StringSharedStorage> {
    if (length.isZero() || !fill.isValidUnicode()) {
        return std::nullopt;
    }
    const auto data = _data.dataSpan();
    const auto currentLength = CpCount::fromSizeT(data.size());
    if (length <= currentLength) {
        return std::nullopt;
    }
    const auto padding = length - currentLength;
    auto leftPadding = CpCount::zero();
    auto rightPadding = CpCount::zero();
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

auto U32StringTransformTools::truncatedIfChanged(
    const CpCount maximumWidth, const TruncateMode mode, const U32StringDataView &ellipsis) const
    -> std::optional<U32StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (maximumWidth.isInfinite()) {
        return std::nullopt;
    }
    if (maximumWidth.isZero()) {
        if (data.empty()) {
            return std::nullopt;
        }
        return U32StringSharedStorage{};
    }
    if (CpCount::fromSizeT(data.size()) <= maximumWidth) {
        return std::nullopt;
    }

    auto ellipsisLength = CpCount::fromSizeT(ellipsis.dataSpan().size());
    auto ellipsisView = ellipsis;
    if (ellipsisLength > maximumWidth) {
        ellipsisLength = CpCount::zero();
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
            dataView(CpRange{suffixStart, CpIndex::fromSizeT(data.size())}.toAbsolute(_data.range().beginIndex())));
        break;
    }
    case TruncateMode::Middle: {
        const auto prefixLength = keepLength.added(CpCount::one()) / 2U;
        const auto suffixLength = keepLength - prefixLength;
        const auto suffixStart = CpIndex::fromSizeT(data.size()).retreated(suffixLength);
        appendTools.append(dataView(CpRange{CpIndex::zero(), prefixLength}.toAbsolute(_data.range().beginIndex())));
        appendTools.append(ellipsisView);
        appendTools.append(
            dataView(CpRange{suffixStart, CpIndex::fromSizeT(data.size())}.toAbsolute(_data.range().beginIndex())));
        break;
    }
    case TruncateMode::End:
    default:
        appendTools.append(dataView(CpRange{CpIndex::zero(), keepLength}.toAbsolute(_data.range().beginIndex())));
        appendTools.append(ellipsisView);
        break;
    }
    return result;
}

auto U32StringTransformTools::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept
    -> CpCount {
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return CpCount::fromSizeT(_data.dataSpan().size());
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
    return CpCount::fromSizeT(length);
}

auto U32StringTransformTools::toEscapedIfChanged(
    const U32String &source, const EscapeFormat format, const EscapeAmount amount) const
    -> std::optional<U32StringEditor> {
    const auto data = _data.dataSpan();
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return std::nullopt;
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto builder = std::optional<AnyStringBuilder>{};
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf32::decodeCharOrReplace(data, position);
        const auto needsEscape = formatter->needsEscape(character, amount);
        const auto needsRepair =
            character == Char::replacement() && !utf32::isValid(data.subspan(begin, position.toSizeT() - begin));
        if (!builder && !needsEscape && !needsRepair) {
            continue;
        }
        if (!builder) {
            builder.emplace(AnyStringBuilder::u32(CpCount::fromSizeT(data.size())));
            builder->append(source.slice(CpRange{CpIndex::zero(), CpCount::fromSizeT(begin)}));
        }
        if (needsEscape) {
            formatter->escape(character, builder.value());
        } else {
            builder->append(character);
        }
    }
    if (builder) {
        return builder->takeU32StringEditor();
    }
    return std::nullopt;
}

auto U32StringTransformTools::dataView(const CpRange range) const -> U32StringDataView {
    return U32StringDataView{_data.data(), range};
}

}
