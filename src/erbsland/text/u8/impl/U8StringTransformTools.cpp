// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringTransformTools.hpp"

#include "U8StringReadTools.hpp"
#include "U8Writer.hpp"

#include "../U8String.hpp"
#include "../U8StringEditor.hpp"

#include "../../../util/impl/LoopControl.hpp"
#include "../../AnyStringBuilder.hpp"
#include "../../impl/escape/EscapeFormatter.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

using namespace unit;

auto U8StringTransformTools::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    auto result = util::LoopResult::Success;
    const auto completed =
        utf8::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            const auto status = function(character);
            if (status == util::LoopStatus::Continue) {
                return true;
            }
            result = util::impl::loopStatusToResult(status);
            return false;
        });
    return completed ? util::LoopResult::Success : result;
}

auto U8StringTransformTools::forEach(const ProcessCharacterWithCpIndexFn &function) const -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    auto result = util::LoopResult::Success;
    auto index = unit::CpIndex::zero();
    const auto completed =
        utf8::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            const auto status = function(character, index);
            if (status == util::LoopStatus::Continue) {
                index.uncheckedIncrement();
                return true;
            }
            result = util::impl::loopStatusToResult(status);
            return false;
        });
    return completed ? util::LoopResult::Success : result;
}

auto U8StringTransformTools::transformedIfChanged(const TransformCharacterFn function) const
    -> std::optional<U8StringSharedStorage> {

    if (function == nullptr) {
        return std::nullopt;
    }

    const auto data = _data.dataSpan();
    auto position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf8::decodeCharOrReplace(data, position);
        const auto mapped = function(character);
        if (mapped.isEndOfData()) {
            return U8StringSharedStorage::fromBytes(data.first(begin), _sensitive);
        }
        if (mapped == character) {
            continue;
        }

        auto reservedSize = begin;
        if (mapped.isValidUnicode()) {
            reservedSize =
                U8StringSharedStorage::checkedAddSize(reservedSize, utf8::encodedLength(mapped).toSizeTOrThrow());
        }
        auto sizingPosition = position;
        while (sizingPosition.toSizeT() < data.size()) {
            const auto sizingCharacter = utf8::decodeCharOrReplace(data, sizingPosition);
            const auto sizingMapped = function(sizingCharacter);
            if (sizingMapped.isEndOfData()) {
                break;
            }
            if (sizingMapped.isValidUnicode()) {
                reservedSize = U8StringSharedStorage::checkedAddSize(
                    reservedSize, utf8::encodedLength(sizingMapped).toSizeTOrThrow());
            }
        }

        auto result = U8StringSharedStorage::forSize(0U, _sensitive);
        if (reservedSize > 0U) {
            result.ensureMutableCapacity(reservedSize);
            if (_sensitive) {
                result.markAsSensitive();
            }
        }
        if (begin > 0U) {
            std::memcpy(result.dataForWrite(), data.data(), begin);
        }

        auto writeSize = begin;
        auto transformPosition = ByteIndex::fromSizeT(begin);
        while (transformPosition.toSizeT() < data.size()) {
            const auto transformCharacter = utf8::decodeCharOrReplace(data, transformPosition);
            const auto transformMapped = function(transformCharacter);
            if (transformMapped.isEndOfData()) {
                break;
            }
            if (!transformMapped.isValidUnicode()) {
                continue;
            }
            const auto encodedSize = utf8::encodedLength(transformMapped).toSizeTOrThrow();
            const auto requiredSize = U8StringSharedStorage::checkedAddSize(writeSize, encodedSize);
            if (requiredSize > result.capacity().toSizeT()) {
                result.ensureMutableCapacity(requiredSize);
                if (_sensitive) {
                    result.markAsSensitive();
                }
            }
            U8Writer{std::span<char>{result.dataForWrite() + writeSize, encodedSize}}.write(transformMapped);
            writeSize = requiredSize;
        }
        result.resize(writeSize);
        return result;
    }
    return std::nullopt;
}

auto U8StringTransformTools::alignedIfChanged(
    const CpCount length, const geometry::Alignment alignment, const Char fill) const
    -> std::optional<U8StringSharedStorage> {
    if (length.isZero() || !fill.isValidUnicode()) {
        return std::nullopt;
    }
    const auto currentLength = U8StringCharReadTool{_data}.charLength();
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
    auto result = U8StringSharedStorage::forSize(0U, _sensitive);
    U8StringAppendTools appendTools{result};
    appendTools.append(fill, leftPadding);
    appendTools.append(_data);
    appendTools.append(fill, rightPadding);
    return result;
}

auto U8StringTransformTools::truncatedIfChanged(
    const CpCount maximumWidth, const TruncateMode mode, const U8StringDataView &ellipsis) const
    -> std::optional<U8StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (maximumWidth.isInfinite()) {
        return std::nullopt;
    }
    if (maximumWidth.isZero()) {
        if (data.empty()) {
            return std::nullopt;
        }
        return U8StringSharedStorage::forSize(0U, _sensitive);
    }
    if (data.size() <= maximumWidth.toSizeT()) {
        return std::nullopt;
    }

    const auto reader = U8StringCharReadTool{_data};
    auto needsTruncation = data.size() / 4U > maximumWidth.toSizeT();
    if (!needsTruncation) {
        auto position = ByteIndex::zero();
        auto count = CpCount::zero();
        while (position.toSizeT() < data.size() && count <= maximumWidth) {
            utf8::fastAdvanceChar(data, position);
            ++count;
        }
        if (count <= maximumWidth) {
            return std::nullopt;
        }
    }

    auto ellipsisLength = U8StringCharReadTool{ellipsis}.charLength();
    auto ellipsisView = ellipsis;
    if (ellipsisLength > maximumWidth) {
        ellipsisLength = CpCount::zero();
        ellipsisView = {};
    }
    const auto keepLength = maximumWidth - ellipsisLength;
    auto result = U8StringSharedStorage::forSize(0U, _sensitive);
    auto appendTools = U8StringAppendTools{result};
    switch (mode) {
    case TruncateMode::Begin: {
        auto suffixStart = ByteIndex::fromSizeT(data.size());
        U8StringReadTools{_data}.retreat(suffixStart, keepLength);
        appendTools.append(ellipsisView);
        appendTools.append(
            dataView(ByteRange{suffixStart, ByteIndex::fromSizeT(data.size())}.toAbsolute(_data.range().beginIndex())));
        break;
    }
    case TruncateMode::Middle: {
        const auto prefixLength = keepLength.added(CpCount::one()) / 2U;
        const auto suffixLength = keepLength - prefixLength;
        auto suffixStart = ByteIndex::fromSizeT(data.size());
        U8StringReadTools{_data}.retreat(suffixStart, suffixLength);
        appendTools.append(dataView(reader.sliceRange(CpRange{CpIndex::zero(), prefixLength})));
        appendTools.append(ellipsisView);
        appendTools.append(
            dataView(ByteRange{suffixStart, ByteIndex::fromSizeT(data.size())}.toAbsolute(_data.range().beginIndex())));
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

auto U8StringTransformTools::escapedSize(const EscapeFormat format, EscapeAmount amount) const noexcept -> ByteCount {
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return ByteCount::fromSizeT(_data.dataSpan().size());
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    ByteCount length;
    utf8::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            length += ByteCount::fromSizeT(formatter->escapeSize(character, StringKind::U8));
        } else {
            length += character.utf8Size();
        }
        return true;
    });
    return length;
}

auto U8StringTransformTools::toEscapedIfChanged(
    const U8String &source, const EscapeFormat format, const EscapeAmount amount) const
    -> std::optional<U8StringEditor> {
    const auto data = _data.dataSpan();
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return std::nullopt;
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto builder = std::optional<AnyStringBuilder>{};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf8::decodeCharOrReplace(data, position);
        const auto needsEscape = formatter->needsEscape(character, amount);
        const auto needsRepair =
            character == Char::replacement() && !utf8::isValid(data.subspan(begin, position.toSizeT() - begin));
        if (!builder && !needsEscape && !needsRepair) {
            continue;
        }
        if (!builder) {
            builder.emplace(AnyStringBuilder::u8(ByteCount::fromSizeT(data.size()), _sensitive));
            builder->append(source.slice(ByteRange{ByteIndex::zero(), ByteCount::fromSizeT(begin)}));
        }
        if (needsEscape) {
            formatter->escape(character, builder.value());
        } else {
            builder->append(character);
        }
    }
    if (builder) {
        return builder->takeU8StringEditor();
    }
    return std::nullopt;
}

auto U8StringTransformTools::dataView(const ByteRange range) const -> U8StringDataView {
    return U8StringDataView{_data.data(), range};
}

}
