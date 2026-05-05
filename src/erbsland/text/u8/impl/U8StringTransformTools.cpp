// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringTransformTools.hpp"

#include "U8StringReadTools.hpp"

#include "../U8String.hpp"

#include "../../../util/impl/LoopControl.hpp"
#include "../../impl/EscapeFormatter.hpp"
#include "../../impl/SafeStringEscapeTools.hpp"
#include "../../StringBuilder.hpp"

#include <string_view>

namespace erbsland::text::impl {

auto U8StringTransformTools::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    auto result = util::LoopResult::Success;
    const auto completed =
        utf8::forEachDecodedCharacter(_data.dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
            const auto status = function(character);
            if (status == util::LoopStatus::Continue) {
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
    auto position = unit::ByteIndex::zero();
    auto result = U8StringSharedStorage{};
    auto appendTools = U8StringAppendTools{result};
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
            appendTools.append(U8StringDataView{data, unit::ByteRange::fromSizeT(unchangedEnd)});
        }
    };

    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf8::decodeCharOrReplace(data, position);
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

auto U8StringTransformTools::transformed(const TransformCharacterFn function) const -> U8StringSharedStorage {
    if (auto result = transformedIfChanged(function)) {
        return std::move(*result);
    }
    return U8StringSharedStorage{_data};
}

auto U8StringTransformTools::aligned(
    const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto currentLength = U8StringCharReadTool{_data}.charLength();
    if (length <= currentLength || !fill.isValidUnicode()) {
        return U8StringSharedStorage::fromBytes(data);
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
    auto result = U8StringSharedStorage{};
    U8StringAppendTools appendTools{result};
    appendTools.append(fill, leftPadding);
    appendTools.append(_data);
    appendTools.append(fill, rightPadding);
    return result;
}

auto U8StringTransformTools::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U8StringDataView &ellipsis) const
    -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (maximumWidth.isInfinite()) {
        return U8StringSharedStorage::fromBytes(data);
    }
    if (maximumWidth.isZero()) {
        return {};
    }
    if (data.size() <= maximumWidth.toSizeT()) {
        return U8StringSharedStorage::fromBytes(data);
    }

    const auto reader = U8StringCharReadTool{_data};
    auto needsTruncation = data.size() / 4U > maximumWidth.toSizeT();
    if (!needsTruncation) {
        auto position = unit::ByteIndex::zero();
        auto count = unit::CpLength::zero();
        while (position.toSizeT() < data.size() && count <= maximumWidth) {
            utf8::fastAdvanceChar(data, position);
            ++count;
        }
        if (count <= maximumWidth) {
            return U8StringSharedStorage::fromBytes(data);
        }
    }

    auto ellipsisLength = U8StringCharReadTool{ellipsis}.charLength();
    auto ellipsisView = ellipsis;
    if (ellipsisLength > maximumWidth) {
        ellipsisLength = unit::CpLength::zero();
        ellipsisView = {};
    }
    const auto keepLength = maximumWidth - ellipsisLength;
    auto result = U8StringSharedStorage{};
    auto appendTools = U8StringAppendTools{result};
    switch (mode) {
    case TruncateMode::Begin: {
        auto suffixStart = unit::ByteIndex::fromSizeT(data.size());
        U8StringReadTools{_data}.retreat(suffixStart, keepLength);
        appendTools.append(ellipsisView);
        appendTools.append(dataView(
            unit::ByteRange{suffixStart, unit::ByteIndex::fromSizeT(data.size())}.withOrigin(_data.range().index())));
        break;
    }
    case TruncateMode::Middle: {
        const auto prefixLength = keepLength.added(unit::CpLength::one()) / 2U;
        const auto suffixLength = keepLength - prefixLength;
        auto suffixStart = unit::ByteIndex::fromSizeT(data.size());
        U8StringReadTools{_data}.retreat(suffixStart, suffixLength);
        appendTools.append(dataView(reader.sliceRange(unit::CpRange{unit::CpIndex::zero(), prefixLength})));
        appendTools.append(ellipsisView);
        appendTools.append(dataView(
            unit::ByteRange{suffixStart, unit::ByteIndex::fromSizeT(data.size())}.withOrigin(_data.range().index())));
        break;
    }
    case TruncateMode::End:
    default:
        appendTools.append(dataView(reader.sliceRange(unit::CpRange{unit::CpIndex::zero(), keepLength})));
        appendTools.append(ellipsisView);
        break;
    }
    return result;
}

auto U8StringTransformTools::escapedSize(const EscapeFormat format, EscapeAmount amount) const noexcept
    -> unit::ByteLength {
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return unit::ByteLength::fromSizeT(_data.dataSpan().size());
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    unit::ByteLength length;
    utf8::forEachDecodedCharacter(_data.dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            length += unit::ByteLength{formatter->escapeSize(character, StringKind::U8)};
        } else {
            length += character.utf8Size();
        }
        return true;
    });
    return length;
}

auto U8StringTransformTools::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U8String {
    const auto data = _data.dataSpan();
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return U8String{std::string_view{data.data(), data.size()}};
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    StringBuilder builder{StringKind::U8};
    utf8::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            formatter->escape(character, builder);
        } else {
            builder.append(character);
        }
        return true;
    });
    return builder.takeU8String();
}

auto U8StringTransformTools::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const
    -> U8String {
    auto scanner = SafeStringEscapeTools{maximumWidth, flags};
    const auto data = _data.dataSpan();
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto sourceStart = position.toSizeT();
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (!scanner.add(character, sourceStart)) {
            break;
        }
    }
    scanner.finish(data.size());
    auto builder = StringBuilder{};
    scanner.appendTo(builder, data.size());
    return builder.takeU8String();
}

auto U8StringTransformTools::dataView(const unit::ByteRange range) const -> U8StringDataView {
    return U8StringDataView{_data.data(), range};
}

}
