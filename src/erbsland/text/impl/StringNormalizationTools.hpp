// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringTraits.hpp"
#include "UnicodeNormalizer.hpp"

#include "../Char.hpp"
#include "../NormalizationForm.hpp"
#include "../u16/impl/U16Encoding.hpp"
#include "../u16/impl/U16StringSharedStorage.hpp"
#include "../u16/impl/U16Writer.hpp"
#include "../u32/impl/U32StringSharedStorage.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/impl/U8StringSharedStorage.hpp"
#include "../u8/impl/U8Writer.hpp"

#include <concepts>
#include <cstring>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

namespace erbsland::text::impl {

/// Normalize one concrete string using bounded stack storage and lazy replacement allocation.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest UnicodeNormalizationAllocationTest}
template <typename tString>
class StringNormalizationTools final {
    static_assert(AnyStringOrStringEditorType<tString>);

private:
    using Source = std::remove_cvref_t<tString>;
    using Traits = StringTraitsFor<Source>;
    using CodeUnit = typename Traits::CodeUnit;
    using SharedStorage = SharedStorageFor<Source>;
    using StringIndex = StringIndexTypeFor<Source>;
    using DataSpan = std::span<const CodeUnit>;

public:
    /// Create a normalizer for one source and explicit normalization form.
    /// @param source The source string or editor whose lifetime must cover this object.
    /// @param form The Unicode normalization form.
    explicit StringNormalizationTools(const Source &source, const NormalizationForm form) noexcept :
        _source{source},
        _data{source.dataView().dataSpan()},
        _sourceEnd{StringIndex::fromSizeT(_data.size())},
        _normalizer{form} {}

    // defaults/deletions
    StringNormalizationTools(const StringNormalizationTools &) = delete;
    StringNormalizationTools(StringNormalizationTools &&) = delete;
    auto operator=(const StringNormalizationTools &) -> StringNormalizationTools & = delete;
    auto operator=(StringNormalizationTools &&) -> StringNormalizationTools & = delete;

public:
    /// Normalize the source and create replacement storage only if its encoded text changes.
    /// @return Replacement storage, or no value when the source storage can be retained.
    [[nodiscard]] auto normalizedIfChanged() -> std::optional<SharedStorage> {
        while (_position < _sourceEnd) {
            const auto sourceBegin = _position;
            auto encodingError = false;
            const auto codePoint = decode(encodingError);
            if (_normalizer.prepare(codePoint)) {
                _normalizer.finish();
                appendWindow(
                    _windowBegin,
                    sourceBegin,
                    _normalizer.normalizedCodePoints(),
                    _windowHasEncodingError || !_normalizer.isUnchanged());
                _normalizer.resetWindow();
                _windowBegin = sourceBegin;
                _windowHasEncodingError = false;
            }
            _normalizer.appendPrepared();
            _windowHasEncodingError = _windowHasEncodingError || encodingError;
        }

        if (!_normalizer.isEmpty()) {
            _normalizer.finish();
            appendWindow(
                _windowBegin,
                _sourceEnd,
                _normalizer.normalizedCodePoints(),
                _windowHasEncodingError || !_normalizer.isUnchanged());
        }
        if (_storage.has_value()) {
            appendRaw(_rawBegin, _sourceEnd);
        }
        return std::move(_storage);
    }

private:
    /// Decode one native-width source scalar and report tolerant replacement.
    [[nodiscard]] auto decode(bool &encodingError) noexcept -> char32_t {
        if constexpr (std::same_as<CodeUnit, char>) {
            const auto character = utf8::tryDecodeChar(_data, _position);
            encodingError = !character.has_value();
            return character.value_or(Char::replacement()).toRawValue();
        } else if constexpr (std::same_as<CodeUnit, char16_t>) {
            const auto character = utf16::tryDecodeChar(_data, _position);
            encodingError = !character.has_value();
            return character.value_or(Char::replacement()).toRawValue();
        } else {
            const auto character = Char{_data[_position.toSizeT()]};
            _position.increment();
            encodingError = !character.isValidUnicode();
            return encodingError ? Char::replacement().toRawValue() : character.toRawValue();
        }
    }
    /// Append one completed normalization window.
    void appendWindow(
        const StringIndex sourceBegin,
        const StringIndex sourceEnd,
        const std::span<const char32_t> normalized,
        const bool changed) {
        if (!changed) {
            return;
        }
        ensureStorage(sourceBegin);
        appendNormalized(normalized);
        _rawBegin = sourceEnd;
    }
    /// Create replacement storage on the first changed window and copy the preceding raw source.
    void ensureStorage(const StringIndex changedBegin) {
        if (_storage.has_value()) {
            appendRaw(_rawBegin, changedBegin);
            return;
        }
        _storage.emplace();
        _storage->ensureMutableCapacity(_data.size());
        if constexpr (std::same_as<CodeUnit, char>) {
            if (_source.isSensitive()) {
                _storage->markAsSensitive();
            }
        }
        appendRaw(StringIndex::zero(), changedBegin);
    }
    /// Copy one unchanged native-unit range into replacement storage.
    void appendRaw(const StringIndex begin, const StringIndex end) {
        if (begin >= end) {
            return;
        }
        const auto beginOffset = begin.toSizeT();
        const auto length = begin.absoluteDistanceTo(end).toSizeT();
        const auto oldSize = _storage->range().length().toSizeT();
        const auto newSize =
            SharedStorage::checkedAddSize(oldSize, length, "Normalized string exceeds string size bounds");
        _storage->ensureMutableCapacity(newSize);
        std::memcpy(_storage->dataForWrite() + oldSize, _data.data() + beginOffset, length * sizeof(CodeUnit));
        _storage->resize(newSize);
    }
    /// Encode one changed normalized scalar window into replacement storage.
    void appendNormalized(const std::span<const char32_t> normalized) {
        auto encodedLength = std::size_t{};
        if constexpr (std::same_as<CodeUnit, char>) {
            for (const auto codePoint : normalized) {
                encodedLength += utf8::encodedLength(Char{codePoint}).toSizeTOrThrow();
            }
        } else if constexpr (std::same_as<CodeUnit, char16_t>) {
            for (const auto codePoint : normalized) {
                encodedLength += utf16::encodedLength(Char{codePoint}).toSizeTOrThrow();
            }
        } else {
            encodedLength = normalized.size();
        }

        const auto oldSize = _storage->range().length().toSizeT();
        const auto newSize =
            SharedStorage::checkedAddSize(oldSize, encodedLength, "Normalized string exceeds string size bounds");
        _storage->ensureMutableCapacity(newSize);
        if constexpr (std::same_as<CodeUnit, char>) {
            auto writer = U8Writer{std::span<char>{_storage->dataForWrite() + oldSize, encodedLength}};
            for (const auto codePoint : normalized) {
                writer.write(Char{codePoint});
            }
        } else if constexpr (std::same_as<CodeUnit, char16_t>) {
            auto writer = U16Writer{std::span<char16_t>{_storage->dataForWrite() + oldSize, encodedLength}};
            for (const auto codePoint : normalized) {
                writer.write(Char{codePoint});
            }
        } else if (encodedLength != 0U) {
            std::memcpy(_storage->dataForWrite() + oldSize, normalized.data(), encodedLength * sizeof(char32_t));
        }
        _storage->resize(newSize);
    }

private:
    const Source &_source;                 ///< Source string whose lifetime covers normalization.
    DataSpan _data;                        ///< Visible native-unit source span.
    StringIndex _sourceEnd;                ///< Native-unit index after the source.
    UnicodeNormalizer _normalizer;         ///< Scalar normalization engine.
    std::optional<SharedStorage> _storage; ///< Lazily allocated replacement storage.
    StringIndex _position;                 ///< Current native-unit decoding position.
    StringIndex _windowBegin;              ///< Native-unit start of the active window.
    StringIndex _rawBegin;                 ///< Native-unit start of the pending unchanged span.
    bool _windowHasEncodingError{};        ///< Whether the active window replaced malformed encoding.
};

template <typename tString>
StringNormalizationTools(const tString &, NormalizationForm) -> StringNormalizationTools<std::remove_cvref_t<tString>>;

}
