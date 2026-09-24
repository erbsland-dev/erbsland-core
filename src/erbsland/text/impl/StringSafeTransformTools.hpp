// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter.hpp"
#include "StringKeyTraits.hpp"
#include "StringTraits.hpp"

#include "../AnyStringBuilder.hpp"
#include "../IntegerBase.hpp"
#include "../SafeStringFlag.hpp"
#include "../u16/impl/U16Encoding.hpp"
#include "../u16/U16Format.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32Format.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/U8Format.hpp"

#include <optional>
#include <span>
#include <type_traits>

namespace erbsland::text::impl {

/// Create a bounded safe representation for one concrete string type.
/// @tested{StringTransformTest}
template <AnyStringOrStringEditorType tString>
class StringSafeTransformTools final {
private:
    using Source = std::remove_cvref_t<tString>;
    using Traits = StringTraitsFor<Source>;
    using CodeUnit = typename Traits::CodeUnit;
    using DataIndex = typename Traits::DataIndex;
    using ReadOnly = typename StringTypes<Source>::ReadOnly;
    using Editor = typename StringTypes<Source>::Editor;

    /// Accumulated rendered and native-size metadata for a source prefix.
    struct ScanInfo {
        std::size_t renderedSize{};
        std::size_t nativeSize{};
        std::size_t quoteTriggers{};
        std::size_t escapes{};
        std::size_t replacements{};
    };

    /// Rendered and native-size metadata for a crop annotation.
    struct SuffixInfo {
        std::size_t renderedSize{};
        std::size_t nativeSize{};
        bool addMark{};
        bool addTotal{};
        bool present{};
    };

public:
    /// Create a transformer for the given source data and output policy.
    StringSafeTransformTools(
        const Source &source,
        const std::span<const CodeUnit> data,
        const unit::CpLength maximumWidth,
        const SafeStringFlags flags) noexcept :
        _source{source},
        _data{data},
        _maximumWidth{maximumWidth},
        _flags{flags},
        _amount{
            flags.isSet(SafeStringFlag::OnlyAscii) ? EscapeAmount{EscapeAmount::NonAscii}
                                                   : EscapeAmount{EscapeAmount::Balanced}},
        _formatter{EscapeFormatter::forFormat(EscapeFormat::Cpp)} {}

    // defaults/deletions
    StringSafeTransformTools(const StringSafeTransformTools &) = delete;
    StringSafeTransformTools(StringSafeTransformTools &&) = delete;
    auto operator=(const StringSafeTransformTools &) -> StringSafeTransformTools & = delete;
    auto operator=(StringSafeTransformTools &&) -> StringSafeTransformTools & = delete;

public:
    /// Create transformed output only if the source representation must change.
    [[nodiscard]] auto toSafeStringIfChanged() -> std::optional<Editor> {
        if (_maximumWidth.isZero()) {
            return _data.empty() ? std::nullopt : std::optional<Editor>{Editor{}};
        }

        scan();
        const auto cropped = !fits(_full, {}, quoteWidth(_full.quoteTriggers != 0U));
        if (!cropped) {
            if (_full.escapes == 0U && _full.replacements == 0U && quoteWidth(_full.quoteTriggers != 0U) == 0U) {
                return std::nullopt;
            }
            _prefix = _full;
            _prefixEnd = DataIndex::fromSizeT(_data.size());
            return build({}, quoteWidth(_prefix.quoteTriggers != 0U));
        }

        const auto suffix = suffixInfo();
        if (!selectPrefix(suffix)) {
            return ellipsis();
        }
        return build(suffix, quoteWidth(suffix.present || _prefix.quoteTriggers != 0U));
    }

private:
    /// Scan the complete source and remember the largest initially fitting prefix.
    void scan() {
        auto position = DataIndex::zero();
        while (position.toSizeT() < _data.size()) {
            add(_full, characterInfo(read(position)));
            ++_totalLength;
            if (fits(_full, {}, quoteWidth(_full.quoteTriggers != 0U))) {
                _prefix = _full;
                _prefixEnd = position;
            }
        }
    }
    /// Retreat from the initial prefix until the crop annotation also fits.
    [[nodiscard]] auto selectPrefix(const SuffixInfo suffix) noexcept -> bool {
        if (!fits({}, suffix, quoteWidth(suffix.present))) {
            return false;
        }
        while (!fits(_prefix, suffix, quoteWidth(suffix.present || _prefix.quoteTriggers != 0U))) {
            const auto character = retreatAndRead();
            remove(_prefix, characterInfo(character));
        }
        return true;
    }
    /// Create metadata for the requested crop annotation.
    [[nodiscard]] auto suffixInfo() const noexcept -> SuffixInfo {
        const auto addMark = _flags.isSet(SafeStringFlag::AddCropMark);
        const auto addTotal = _flags.isSet(SafeStringFlag::AddTotalsOnCrop);
        const auto digits = addTotal ? IntegerBase{IntegerBase::Decimal}.digitCount(_totalLength) : 0U;
        return SuffixInfo{
            .renderedSize = static_cast<std::size_t>(addMark) + (addTotal ? digits + 8U : 0U),
            .nativeSize = (addMark ? encodedSize(Char{U'…'}) : 0U) + (addTotal ? digits + 8U : 0U),
            .addMark = addMark,
            .addTotal = addTotal,
            .present = addMark || addTotal,
        };
    }
    /// Create output metadata for one decoded source character.
    [[nodiscard]] auto characterInfo(const Char character) const noexcept -> ScanInfo {
        const auto escaped = needsEscape(character);
        const auto escapedSize = escaped ? _formatter->escapeSize(character, Traits::kind) : 0U;
        return ScanInfo{
            .renderedSize = escaped ? escapedSize : 1U,
            .nativeSize = escaped ? escapedSize : encodedSize(character),
            .quoteTriggers = static_cast<std::size_t>(escaped || character.isAsciiWhitespace() || character == U'"'),
            .escapes = static_cast<std::size_t>(escaped),
            .replacements = static_cast<std::size_t>(character == Char::replacement()),
        };
    }
    /// Get the rendered width of automatic enclosing quotes.
    [[nodiscard]] auto quoteWidth(const bool needed) const noexcept -> std::size_t {
        return _flags.isSet(SafeStringFlag::AutoQuotes) && needed ? 2U : 0U;
    }
    /// Test whether body, suffix, and quotes fit the requested maximum width.
    [[nodiscard]] auto fits(const ScanInfo body, const SuffixInfo suffix, const std::size_t quotes) const noexcept
        -> bool {
        if (_maximumWidth.isInfinite()) {
            return true;
        }
        const auto maximum = _maximumWidth.toSizeT();
        return body.renderedSize <= maximum && suffix.renderedSize <= maximum - body.renderedSize &&
            quotes <= maximum - body.renderedSize - suffix.renderedSize;
    }
    /// Test whether one character must be escaped under the selected policy.
    [[nodiscard]] auto needsEscape(const Char character) const noexcept -> bool {
        return _formatter->needsEscape(character, _amount);
    }
    /// Get the target native-unit size for one valid decoded character.
    [[nodiscard]] static auto encodedSize(const Char character) noexcept -> std::size_t {
        if constexpr (Traits::kind == StringKind::U8) {
            return utf8::encodedLength(character).toSizeTOrThrow();
        } else if constexpr (Traits::kind == StringKind::U16) {
            return utf16::encodedLength(character).toSizeTOrThrow();
        } else {
            return 1U;
        }
    }
    /// Build the selected prefix, crop annotation, and enclosing quotes.
    [[nodiscard]] auto build(const SuffixInfo suffix, const std::size_t quotes) -> Editor {
        auto builder = builderWithCapacity(_prefix.nativeSize + suffix.nativeSize + quotes);
        if (quotes != 0U) {
            builder.append(U'"');
        }
        appendBody(builder);
        appendSuffix(builder, suffix);
        if (quotes != 0U) {
            builder.append(U'"');
        }
        return takeEditor(builder);
    }
    /// Build the unconditional tiny-width fallback.
    [[nodiscard]] auto ellipsis() const -> Editor {
        auto builder = builderWithCapacity(encodedSize(Char{U'…'}));
        builder.append(U'…');
        return takeEditor(builder);
    }
    /// Append the selected prefix directly or as rebuilt escaped text.
    void appendBody(AnyStringBuilder &builder) const {
        if (_prefix.escapes == 0U && _prefix.replacements == 0U) {
            builder.append(ReadOnly{_source}.slice(StringSide::Front, _prefixEnd));
            return;
        }
        auto position = DataIndex::zero();
        while (position < _prefixEnd) {
            const auto character = read(position);
            if (needsEscape(character)) {
                _formatter->escape(character, builder);
            } else {
                builder.append(character);
            }
        }
    }
    /// Append the selected crop annotation using the target-width formatter.
    void appendSuffix(AnyStringBuilder &builder, const SuffixInfo suffix) const {
        if (suffix.addMark) {
            builder.append(U'…');
        }
        if (!suffix.addTotal) {
            return;
        }
        if constexpr (Traits::kind == StringKind::U8) {
            static const auto cTotalFormat = U8Format{"({} total)"};
            cTotalFormat.appendTo(builder, _totalLength);
        } else if constexpr (Traits::kind == StringKind::U16) {
            static const auto cTotalFormat = U16Format{u"({} total)"};
            cTotalFormat.appendTo(builder, _totalLength);
        } else {
            static const auto cTotalFormat = U32Format{U"({} total)"};
            cTotalFormat.appendTo(builder, _totalLength);
        }
    }
    /// Tolerantly decode one character and advance the given native index.
    [[nodiscard]] auto read(DataIndex &position) const noexcept -> Char {
        if constexpr (Traits::kind == StringKind::U8) {
            return utf8::decodeCharOrReplace(_data, position);
        } else if constexpr (Traits::kind == StringKind::U16) {
            return utf16::decodeCharOrReplace(_data, position);
        } else {
            return utf32::decodeCharOrReplace(_data, position);
        }
    }
    /// Retreat the prefix boundary by one decoded character and return it.
    [[nodiscard]] auto retreatAndRead() noexcept -> Char {
        if constexpr (Traits::kind == StringKind::U8) {
            utf8::fastRetreatChar(_data, _prefixEnd);
        } else if constexpr (Traits::kind == StringKind::U16) {
            utf16::fastRetreatChar(_data, _prefixEnd);
        } else {
            --_prefixEnd;
        }
        auto position = _prefixEnd;
        return read(position);
    }
    /// Create a target-width builder with the exact calculated native capacity.
    [[nodiscard]] static auto builderWithCapacity(const std::size_t nativeSize) -> AnyStringBuilder {
        if constexpr (Traits::kind == StringKind::U8) {
            return AnyStringBuilder::u8(unit::ByteLength::fromSizeTOrThrow(nativeSize));
        } else if constexpr (Traits::kind == StringKind::U16) {
            return AnyStringBuilder::u16(unit::U16DataLength::fromSizeTOrThrow(nativeSize));
        } else {
            return AnyStringBuilder::u32(unit::CpLength::fromSizeTOrThrow(nativeSize));
        }
    }
    /// Move the target-width editor out of a completed builder.
    [[nodiscard]] static auto takeEditor(AnyStringBuilder &builder) -> Editor {
        if constexpr (Traits::kind == StringKind::U8) {
            return builder.takeU8StringEditor();
        } else if constexpr (Traits::kind == StringKind::U16) {
            return builder.takeU16StringEditor();
        } else {
            return builder.takeU32StringEditor();
        }
    }
    /// Add character or prefix metadata to an accumulator.
    static void add(ScanInfo &target, const ScanInfo addition) noexcept {
        target.renderedSize += addition.renderedSize;
        target.nativeSize += addition.nativeSize;
        target.quoteTriggers += addition.quoteTriggers;
        target.escapes += addition.escapes;
        target.replacements += addition.replacements;
    }
    /// Subtract removed character metadata from an accumulator.
    static void remove(ScanInfo &target, const ScanInfo subtraction) noexcept {
        target.renderedSize -= subtraction.renderedSize;
        target.nativeSize -= subtraction.nativeSize;
        target.quoteTriggers -= subtraction.quoteTriggers;
        target.escapes -= subtraction.escapes;
        target.replacements -= subtraction.replacements;
    }

private:
    const Source &_source;
    std::span<const CodeUnit> _data;
    unit::CpLength _maximumWidth;
    SafeStringFlags _flags;
    EscapeAmount _amount;
    EscapeFormatterPtr _formatter;
    DataIndex _prefixEnd;
    ScanInfo _full;
    ScanInfo _prefix;
    std::size_t _totalLength{};
};

template <typename tString>
StringSafeTransformTools(
    const tString &,
    std::span<const typename StringTraitsFor<std::remove_cvref_t<tString>>::CodeUnit>,
    unit::CpLength,
    SafeStringFlags) -> StringSafeTransformTools<std::remove_cvref_t<tString>>;

}
