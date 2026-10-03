// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringTransformTools.hpp"

#include "U16StringReadTools.hpp"
#include "U16Writer.hpp"

#include "../U16String.hpp"
#include "../U16StringEditor.hpp"

#include "../../../util/impl/LoopControl.hpp"
#include "../../AnyStringBuilder.hpp"
#include "../../impl/escape/EscapeFormatter.hpp"

#include <cstring>
#include <span>

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
    auto position = U16Index::zero();
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
            reservedSize =
                U16StringSharedStorage::checkedAddSize(reservedSize, utf16::encodedLength(mapped).toSizeTOrThrow());
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
                    reservedSize, utf16::encodedLength(sizingMapped).toSizeTOrThrow());
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
        auto transformPosition = U16Index::fromSizeT(begin);
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
            const auto requiredSize = U16StringSharedStorage::checkedAddSize(writeSize, encodedSize);
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

auto U16StringTransformTools::alignedIfChanged(
    const CpCount length, const geometry::Alignment alignment, const Char fill) const
    -> std::optional<U16StringSharedStorage> {
    if (length.isZero() || !fill.isValidUnicode()) {
        return std::nullopt;
    }
    const auto currentLength = U16StringCharReadTool{_data}.charLength();
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
    auto result = U16StringSharedStorage{};
    U16StringAppendTools appendTools{result};
    appendTools.append(fill, leftPadding);
    appendTools.append(_data);
    appendTools.append(fill, rightPadding);
    return result;
}

auto U16StringTransformTools::truncatedIfChanged(
    const CpCount maximumWidth, const TruncateMode mode, const U16StringDataView &ellipsis) const
    -> std::optional<U16StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (maximumWidth.isInfinite()) {
        return std::nullopt;
    }
    if (maximumWidth.isZero()) {
        if (data.empty()) {
            return std::nullopt;
        }
        return U16StringSharedStorage{};
    }
    if (data.size() <= maximumWidth.toSizeT()) {
        return std::nullopt;
    }

    const auto reader = U16StringCharReadTool{_data};
    auto needsTruncation = data.size() / 2U > maximumWidth.toSizeT();
    if (!needsTruncation) {
        auto position = U16Index::zero();
        auto count = CpCount::zero();
        while (position.toSizeT() < data.size() && count <= maximumWidth) {
            utf16::fastAdvanceChar(data, position);
            ++count;
        }
        if (count <= maximumWidth) {
            return std::nullopt;
        }
    }

    auto ellipsisLength = U16StringCharReadTool{ellipsis}.charLength();
    auto ellipsisView = ellipsis;
    if (ellipsisLength > maximumWidth) {
        ellipsisLength = CpCount::zero();
        ellipsisView = {};
    }
    const auto keepLength = maximumWidth - ellipsisLength;
    auto result = U16StringSharedStorage{};
    auto appendTools = U16StringAppendTools{result};
    switch (mode) {
    case TruncateMode::Begin: {
        auto suffixStart = U16Index::fromSizeT(data.size());
        U16StringReadTools{_data}.retreat(suffixStart, keepLength);
        appendTools.append(ellipsisView);
        appendTools.append(
            dataView(U16Range{suffixStart, U16Index::fromSizeT(data.size())}.toAbsolute(_data.range().beginIndex())));
        break;
    }
    case TruncateMode::Middle: {
        const auto prefixLength = keepLength.added(CpCount::one()) / 2U;
        const auto suffixLength = keepLength - prefixLength;
        auto suffixStart = U16Index::fromSizeT(data.size());
        U16StringReadTools{_data}.retreat(suffixStart, suffixLength);
        appendTools.append(dataView(reader.sliceRange(CpRange{CpIndex::zero(), prefixLength})));
        appendTools.append(ellipsisView);
        appendTools.append(
            dataView(U16Range{suffixStart, U16Index::fromSizeT(data.size())}.toAbsolute(_data.range().beginIndex())));
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
    -> U16Count {
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return U16Count::fromSizeT(_data.dataSpan().size());
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
    return U16Count::fromSizeT(length);
}

auto U16StringTransformTools::toEscapedIfChanged(
    const U16String &source, const EscapeFormat format, const EscapeAmount amount) const
    -> std::optional<U16StringEditor> {
    const auto data = _data.dataSpan();
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return std::nullopt;
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto builder = std::optional<AnyStringBuilder>{};
    auto position = U16Index::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf16::decodeCharOrReplace(data, position);
        const auto needsEscape = formatter->needsEscape(character, amount);
        const auto needsRepair =
            character == Char::replacement() && !utf16::isValid(data.subspan(begin, position.toSizeT() - begin));
        if (!builder && !needsEscape && !needsRepair) {
            continue;
        }
        if (!builder) {
            builder.emplace(AnyStringBuilder::u16(U16Count::fromSizeT(data.size())));
            builder->append(source.slice(U16Range{U16Index::zero(), U16Count::fromSizeT(begin)}));
        }
        if (needsEscape) {
            formatter->escape(character, builder.value());
        } else {
            builder->append(character);
        }
    }
    if (builder) {
        return builder->takeU16StringEditor();
    }
    return std::nullopt;
}

auto U16StringTransformTools::dataView(const U16Range range) const -> U16StringDataView {
    return U16StringDataView{_data.data(), range};
}

}
