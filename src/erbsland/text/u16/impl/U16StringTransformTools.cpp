// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringTransformTools.hpp"

#include "U16StringReadTools.hpp"

#include "../U16String.hpp"

#include "../../../util/impl/LoopControl.hpp"
#include "../../impl/EscapeFormatter.hpp"
#include "../../impl/SafeStringEscapeTools.hpp"
#include "../../StringBuilder.hpp"

#include <string_view>

namespace erbsland::text::impl {

auto U16StringTransformTools::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    auto result = util::LoopResult::Success;
    const auto completed =
        utf16::forEachDecodedCharacter(_data.dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
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
    auto position = unit::U16DataIndex::zero();
    auto result = U16StringSharedStorage{};
    auto appendTools = U16StringAppendTools{result};
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
            appendTools.append(U16StringDataView{data, unit::U16DataRange::fromSizeT(unchangedEnd)});
        }
    };

    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf16::decodeCharOrReplace(data, position);
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

auto U16StringTransformTools::transformed(const TransformCharacterFn function) const -> U16StringSharedStorage {
    if (auto result = transformedIfChanged(function)) {
        return std::move(*result);
    }
    return U16StringSharedStorage{_data};
}

auto U16StringTransformTools::aligned(
    const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto currentLength = U16StringCharReadTool{_data}.charLength();
    if (length <= currentLength || !fill.isValidUnicode()) {
        return U16StringSharedStorage::fromCodeUnits(data);
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
    auto result = U16StringSharedStorage{};
    U16StringAppendTools appendTools{result};
    appendTools.append(fill, leftPadding);
    appendTools.append(_data);
    appendTools.append(fill, rightPadding);
    return result;
}

auto U16StringTransformTools::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U16StringDataView &ellipsis) const
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
        auto position = unit::U16DataIndex::zero();
        auto count = unit::CpLength::zero();
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
        ellipsisLength = unit::CpLength::zero();
        ellipsisView = {};
    }
    const auto keepLength = maximumWidth - ellipsisLength;
    auto result = U16StringSharedStorage{};
    auto appendTools = U16StringAppendTools{result};
    switch (mode) {
    case TruncateMode::Begin: {
        auto suffixStart = unit::U16DataIndex::fromSizeT(data.size());
        U16StringReadTools{_data}.retreat(suffixStart, keepLength);
        appendTools.append(ellipsisView);
        appendTools.append(dataView(
            unit::U16DataRange{suffixStart, unit::U16DataIndex::fromSizeT(data.size())}.withOrigin(
                _data.range().index())));
        break;
    }
    case TruncateMode::Middle: {
        const auto prefixLength = keepLength.added(unit::CpLength::one()) / 2U;
        const auto suffixLength = keepLength - prefixLength;
        auto suffixStart = unit::U16DataIndex::fromSizeT(data.size());
        U16StringReadTools{_data}.retreat(suffixStart, suffixLength);
        appendTools.append(dataView(reader.sliceRange(unit::CpRange{unit::CpIndex::zero(), prefixLength})));
        appendTools.append(ellipsisView);
        appendTools.append(dataView(
            unit::U16DataRange{suffixStart, unit::U16DataIndex::fromSizeT(data.size())}.withOrigin(
                _data.range().index())));
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

auto U16StringTransformTools::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept
    -> unit::U16DataLength {
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return unit::U16DataLength::fromSizeT(_data.dataSpan().size());
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto length = std::size_t{0U};
    utf16::forEachDecodedCharacter(_data.dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            length += formatter->escapeSize(character, StringKind::U16);
        } else {
            length += character.encodedSize(StringKind::U16);
        }
        return true;
    });
    return unit::U16DataLength::fromSizeT(length);
}

auto U16StringTransformTools::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U16String {
    const auto data = _data.dataSpan();
    if (format == EscapeFormat::None || amount == EscapeAmount::Nothing) {
        return U16String{std::u16string_view{data.data(), data.size()}};
    }
    const auto formatter = EscapeFormatter::forFormat(format);
    auto builder = StringBuilder{StringKind::U16};
    utf16::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> bool {
        if (formatter->needsEscape(character, amount)) {
            formatter->escape(character, builder);
        } else {
            builder.append(character);
        }
        return true;
    });
    return builder.takeU16String();
}

auto U16StringTransformTools::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const
    -> U16String {
    auto scanner = SafeStringEscapeTools{maximumWidth, flags};
    const auto data = _data.dataSpan();
    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto sourceStart = position.toSizeT();
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (!scanner.add(character, sourceStart)) {
            break;
        }
    }
    scanner.finish(data.size());
    auto builder = StringBuilder{StringKind::U16};
    scanner.appendTo(builder, data.size());
    return builder.takeU16String();
}

auto U16StringTransformTools::dataView(const unit::U16DataRange range) const -> U16StringDataView {
    return U16StringDataView{_data.data(), range};
}

}
