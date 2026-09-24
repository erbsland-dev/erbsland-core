// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringTransformTools.hpp"

#include "U16StringReadTools.hpp"
#include "U16Writer.hpp"

#include "../U16StringEditor.hpp"

#include "../../../util/impl/LoopControl.hpp"
#include "../../AnyStringBuilder.hpp"
#include "../../impl/EscapeFormatter.hpp"

#include <cstring>
#include <span>
#include <string_view>

namespace erbsland::text::impl {

using namespace unit;

auto U16StringTransformTools::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    auto result = util::LoopResult::Success;
    const auto completed =
        utf16::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            const auto status = function(character);
            if (status == util::LoopStatus::Continue) {
                return true;
            }
            result = util::impl::loopStatusToResult(status);
            return false;
        });
    return completed ? util::LoopResult::Success : result;
}

auto U16StringTransformTools::transformedIfChanged(const TransformCharacterFn function) const
    -> std::optional<U16StringSharedStorage> {

    if (function == nullptr) {
        return std::nullopt;
    }

    const auto data = _data.dataSpan();
    auto position = U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf16::decodeCharOrReplace(data, position);
        const auto mapped = function(character);
        if (mapped.isEndOfData()) {
            return U16StringSharedStorage::fromCodeUnits(data.first(begin));
        }
        if (mapped == character) {
            continue;
        }

        auto reservedSize = begin;
        if (mapped.isValidUnicode()) {
            reservedSize = U16StringSharedStorage::checkedAddSize(
                reservedSize, utf16::encodedLength(mapped).toSizeTOrThrow(), "Transformed string exceeds size bounds");
        }
        auto sizingPosition = position;
        while (sizingPosition.toSizeT() < data.size()) {
            const auto sizingCharacter = utf16::decodeCharOrReplace(data, sizingPosition);
            const auto sizingMapped = function(sizingCharacter);
            if (sizingMapped.isEndOfData()) {
                break;
            }
            if (sizingMapped.isValidUnicode()) {
                reservedSize = U16StringSharedStorage::checkedAddSize(
                    reservedSize,
                    utf16::encodedLength(sizingMapped).toSizeTOrThrow(),
                    "Transformed string exceeds size bounds");
            }
        }

        auto result = U16StringSharedStorage{};
        if (reservedSize > 0U) {
            result.ensureMutableCapacity(reservedSize);
        }
        if (begin > 0U) {
            std::memcpy(result.dataForWrite(), data.data(), begin * sizeof(char16_t));
        }

        auto writeSize = begin;
        auto transformPosition = U16DataIndex::fromSizeT(begin);
        while (transformPosition.toSizeT() < data.size()) {
            const auto transformCharacter = utf16::decodeCharOrReplace(data, transformPosition);
            const auto transformMapped = function(transformCharacter);
            if (transformMapped.isEndOfData()) {
                break;
            }
            if (!transformMapped.isValidUnicode()) {
                continue;
            }
            const auto encodedSize = utf16::encodedLength(transformMapped).toSizeTOrThrow();
            const auto requiredSize = U16StringSharedStorage::checkedAddSize(
                writeSize, encodedSize, "Transformed string exceeds size bounds");
            if (requiredSize > result.capacity().toSizeT()) {
                result.ensureMutableCapacity(requiredSize);
            }
            U16Writer{std::span<char16_t>{result.dataForWrite() + writeSize, encodedSize}}.write(transformMapped);
            writeSize = requiredSize;
        }
        result.resize(writeSize);
        return result;
    }
    return std::nullopt;
}

auto U16StringTransformTools::aligned(const CpLength length, const geometry::Alignment alignment, const Char fill) const
    -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto currentLength = U16StringCharReadTool{_data}.charLength();
    if (length <= currentLength || !fill.isValidUnicode()) {
        return U16StringSharedStorage::fromCodeUnits(data);
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
    auto result = U16StringSharedStorage{};
    U16StringAppendTools appendTools{result};
    appendTools.append(fill, leftPadding);
    appendTools.append(_data);
    appendTools.append(fill, rightPadding);
    return result;
}

auto U16StringTransformTools::truncated(
    const CpLength maximumWidth, const TruncateMode mode, const U16StringDataView &ellipsis) const
    -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    if (maximumWidth.isInfinite()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }
    if (maximumWidth.isZero()) {
        return {};
    }
    if (data.size() <= maximumWidth.toSizeT()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }

    const auto reader = U16StringCharReadTool{_data};
    auto needsTruncation = data.size() / 2U > maximumWidth.toSizeT();
    if (!needsTruncation) {
        auto position = U16DataIndex::zero();
        auto count = CpLength::zero();
        while (position.toSizeT() < data.size() && count <= maximumWidth) {
            utf16::fastAdvanceChar(data, position);
            ++count;
        }
        if (count <= maximumWidth) {
            return U16StringSharedStorage::fromCodeUnits(data);
        }
    }

    auto ellipsisLength = U16StringCharReadTool{ellipsis}.charLength();
    auto ellipsisView = ellipsis;
    if (ellipsisLength > maximumWidth) {
        ellipsisLength = CpLength::zero();
        ellipsisView = {};
    }
    const auto keepLength = maximumWidth - ellipsisLength;
    auto result = U16StringSharedStorage{};
    auto appendTools = U16StringAppendTools{result};
    switch (mode) {
    case TruncateMode::Begin: {
        auto suffixStart = U16DataIndex::fromSizeT(data.size());
        U16StringReadTools{_data}.retreat(suffixStart, keepLength);
        appendTools.append(ellipsisView);
        appendTools.append(dataView(
            U16DataRange{suffixStart, U16DataIndex::fromSizeT(data.size())}.withOrigin(_data.range().index())));
        break;
    }
    case TruncateMode::Middle: {
        const auto prefixLength = keepLength.added(CpLength::one()) / 2U;
        const auto suffixLength = keepLength - prefixLength;
        auto suffixStart = U16DataIndex::fromSizeT(data.size());
        U16StringReadTools{_data}.retreat(suffixStart, suffixLength);
        appendTools.append(dataView(reader.sliceRange(CpRange{CpIndex::zero(), prefixLength})));
        appendTools.append(ellipsisView);
        appendTools.append(dataView(
            U16DataRange{suffixStart, U16DataIndex::fromSizeT(data.size())}.withOrigin(_data.range().index())));
        break;
    }
    case TruncateMode::End:
    default:
        appendTools.append(dataView(reader.sliceRange(CpRange{CpIndex::zero(), keepLength})));
        appendTools.append(ellipsisView);
        break;
    }
    return result;
}

auto U16StringTransformTools::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept
    -> U16DataLength {
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return U16DataLength::fromSizeT(_data.dataSpan().size());
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto length = std::size_t{0U};
    utf16::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            length += formatter->escapeSize(character, StringKind::U16);
        } else {
            length += character.encodedSize(StringKind::U16);
        }
        return true;
    });
    return U16DataLength::fromSizeT(length);
}

auto U16StringTransformTools::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U16StringEditor {
    const auto data = _data.dataSpan();
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return U16StringEditor{std::u16string_view{data.data(), data.size()}};
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto builder = AnyStringBuilder{StringKind::U16};
    utf16::forEachDecodedCharacter(data, EncodingMode::Tolerant, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            formatter->escape(character, builder);
        } else {
            builder.append(character);
        }
        return true;
    });
    return builder.takeU16StringEditor();
}

auto U16StringTransformTools::dataView(const U16DataRange range) const -> U16StringDataView {
    return U16StringDataView{_data.data(), range};
}

}
