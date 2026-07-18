// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringEditor.hpp"

#include "U8String.hpp"
#include "U8StringConstIterator.hpp"
#include "U8StringEditorList.hpp"
#include "U8StringList.hpp"
#include "U8StringLiteral.hpp"

#include "impl/U8Encoding.hpp"
#include "impl/U8StringAppendTools.hpp"
#include "impl/U8StringCharReadTool.hpp"
#include "impl/U8StringComparisonTools.hpp"
#include "impl/U8StringData.hpp"
#include "impl/U8StringEncodingTools.hpp"
#include "impl/U8StringModifyTools.hpp"
#include "impl/U8StringReadTools.hpp"
#include "impl/U8StringTransformTools.hpp"
#include "impl/U8Writer.hpp"

#include "../u16/impl/U16Encoding.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

using namespace impl;
using namespace unit;

auto U8StringEditor::clear() noexcept -> U8StringEditor & {
    _storage.clear();
    return *this;
}

auto U8StringEditor::append(const U8String &text, const ElementCount count) -> U8StringEditor & {
    U8StringAppendTools{_storage}.append(text.dataView(), count);
    return *this;
}

auto U8StringEditor::append(const Char character, CpLength count) -> U8StringEditor & {
    U8StringAppendTools{_storage}.append(character, count);
    return *this;
}

auto U8StringEditor::remove(const ByteRange range) -> U8StringEditor & {
    U8StringModifyTools::remove(_storage, range);
    return *this;
}

auto U8StringEditor::remove(const CpRange range) -> U8StringEditor & {
    U8StringModifyTools::remove(_storage, range);
    return *this;
}

auto U8StringEditor::removeAll(const CharSet &characters) -> U8StringEditor & {
    U8StringModifyTools::removeAll(_storage, characters);
    return *this;
}

auto U8StringEditor::removeAll(const U8String &text, const CharCompareFn compareFn) -> U8StringEditor & {
    U8StringModifyTools::remove(_storage, text.dataView(), compareFn);
    return *this;
}

auto U8StringEditor::removeFirst(const U8String &text, const CharCompareFn compareFn) -> U8StringEditor & {
    U8StringModifyTools::removeFirst(_storage, text.dataView(), compareFn);
    return *this;
}

auto U8StringEditor::keep(const ByteRange range) -> U8StringEditor & {
    U8StringModifyTools::keep(_storage, range);
    return *this;
}

auto U8StringEditor::keep(const CpRange range) -> U8StringEditor & {
    U8StringModifyTools::keep(_storage, range);
    return *this;
}

auto U8StringEditor::insert(const ByteIndex index, const U8String &text) -> U8StringEditor & {
    U8StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U8StringEditor::insert(const CpIndex index, const U8String &text) -> U8StringEditor & {
    U8StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U8StringEditor::replace(const ByteRange range, const U8String &text) -> U8StringEditor & {
    U8StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U8StringEditor::replace(const CpRange range, const U8String &text) -> U8StringEditor & {
    U8StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U8StringEditor::replaceFirst(const U8String &text, const U8String &replacement, const CharCompareFn compareFn)
    -> U8StringEditor & {
    U8StringModifyTools::replaceFirst(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U8StringEditor::replaceAll(const CharSet &characters, const Char replacement) -> U8StringEditor & {
    U8StringModifyTools::replaceAll(_storage, characters, replacement);
    return *this;
}

auto U8StringEditor::replaceAll(const CharSet &characters, const U8String &replacement) -> U8StringEditor & {
    U8StringModifyTools::replaceAll(_storage, characters, replacement.dataView());
    return *this;
}

auto U8StringEditor::replaceAll(const U8String &text, const U8String &replacement, const CharCompareFn compareFn)
    -> U8StringEditor & {
    U8StringModifyTools::replaceAll(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U8StringEditor::truncate(const CpLength maximumWidth, const TruncateMode mode) -> U8StringEditor & {
    return truncate(maximumWidth, mode, U8String{});
}

auto U8StringEditor::truncate(const CpLength maximumWidth, const TruncateMode mode, const U8String &ellipsis)
    -> U8StringEditor & {
    *this = truncated(maximumWidth, mode, ellipsis);
    return *this;
}

auto U8StringEditor::removed(const ByteRange range) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.removed(range)};
}

auto U8StringEditor::removed(const CpRange range) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.removed(range)};
}

auto U8StringEditor::removedAll(const CharSet &characters) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.removedAll(characters)};
}

auto U8StringEditor::removedAll(const U8String &text, const CharCompareFn compareFn) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U8StringEditor::removedFirst(const U8String &text, const CharCompareFn compareFn) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U8StringEditor::kept(const ByteRange range) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.kept(range)};
}

auto U8StringEditor::kept(const CpRange range) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.kept(range)};
}

auto U8StringEditor::inserted(const ByteIndex index, const U8String &text) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8StringEditor::inserted(const CpIndex index, const U8String &text) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8StringEditor::replaced(const ByteRange range, const U8String &text) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8StringEditor::replaced(const CpRange range, const U8String &text) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8StringEditor::replacedFirst(
    const U8String &text, const U8String &replacement, const CharCompareFn compareFn) const -> U8StringEditor {
    return U8StringEditor{
        U8StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8StringEditor::replacedAll(const CharSet &characters, const Char replacement) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U8StringEditor::replacedAll(const CharSet &characters, const U8String &replacement) const -> U8StringEditor {
    return U8StringEditor{U8StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U8StringEditor::replacedAll(const U8String &text, const U8String &replacement, const CharCompareFn compareFn) const
    -> U8StringEditor {
    return U8StringEditor{
        U8StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8StringEditor::truncated(const CpLength maximumWidth, const TruncateMode mode) const -> U8StringEditor {
    return truncated(maximumWidth, mode, U8String{});
}

auto U8StringEditor::truncated(const CpLength maximumWidth, const TruncateMode mode, const U8String &ellipsis) const
    -> U8StringEditor {
    return U8StringEditor{U8StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U8StringEditor::aligned(const CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U8StringEditor {
    return U8StringEditor{U8StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U8StringEditor::toSafeString(const CpLength maximumWidth, const SafeStringFlags flags) const -> U8StringEditor {
    return U8StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

void swap(U8StringEditor &first, U8StringEditor &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

void U8StringEditor::reset() noexcept {
    _storage = {};
}

auto U8StringEditor::storageId() const noexcept -> mem::StorageIdentifier {
    return _storage.storageId();
}

void U8StringEditor::reserve(const ByteLength capacity) {
    _storage.reserve(capacity);
}

void U8StringEditor::shrinkToFit() {
    _storage.shrinkToFit();
}

auto U8StringEditor::capacity() const noexcept -> ByteLength {
    return _storage.capacity();
}

auto U8StringEditor::memoryUsage() const noexcept -> ByteLength {
    return _storage.memoryUsage();
}

auto U8StringEditor::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> ByteLength {
    return U8StringTransformTools{_storage.dataView()}.escapedSize(format, amount);
}

auto U8StringEditor::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U8StringEditor {
    return U8StringTransformTools{_storage.dataView()}.toEscaped(format, amount);
}

auto U8StringEditor::fromCharacter(const Char character, const CpLength count) -> U8StringEditor {
    auto storage = U8StringSharedStorage{};
    U8StringAppendTools{storage}.append(character, count);
    return U8StringEditor{std::move(storage)};
}

auto U8StringEditor::fromJoined(const std::initializer_list<U8String> parts) -> U8StringEditor {
    auto finalSize = std::size_t{0};
    for (const auto &part : parts) {
        finalSize = U8StringSharedStorage::checkedAddSize(
            finalSize, part.length().toSizeT(), "Joined string exceeds size bounds");
    }
    if (finalSize == 0U) {
        return {};
    }

    auto storage = U8StringSharedStorage::forSize(finalSize);
    auto writePosition = std::size_t{0};
    for (const auto &part : parts) {
        const auto data = part.dataView().dataSpan();
        if (data.empty()) {
            continue;
        }
        std::memcpy(storage.dataForWrite() + writePosition, data.data(), data.size());
        writePosition = U8StringSharedStorage::checkedAddSize(
            writePosition, data.size(), "Joined string write exceeds size bounds");
    }
    return U8StringEditor{std::move(storage)};
}

auto U8StringEditor::fromBoolean(const bool value, const BooleanFormat format) -> U8StringEditor {
    return U8StringEditor{format.text(value)};
}

auto U8StringEditor::fromByteBlock(const mem::ByteBlock &bytes, const ByteFormat format) -> U8StringEditor {
    auto builder = AnyStringBuilder{};
    builder.appendByteBlock(bytes, format);
    return builder.takeU8StringEditor();
}

auto U8StringEditor::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
            util::advanceHash(result, character.toRawValue());
            return true;
        });
    return result;
}

auto U8StringEditor::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
            util::advanceHash(result, character.caseFolded().toRawValue());
            return true;
        });
    return result;
}

auto U8StringEditor::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return U8StringTransformTools{dataView()}.forEach(function);
}

auto U8StringEditor::transformed(const TransformCharacterFn function) const -> U8StringEditor {
    if (auto result = U8StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U8StringEditor{std::move(*result)};
    }
    return *this;
}

void U8StringEditor::detach() {
    _storage.detach();
}

auto U8StringEditor::begin() const noexcept -> const_iterator {
    return U8StringConstIterator{U8String{*this}, indexAt(StringSide::Front)};
}

auto U8StringEditor::end() const noexcept -> const_iterator {
    return U8StringConstIterator{U8String{*this}, indexAt(StringSide::Back)};
}

auto U8StringEditor::withRange(const ByteRange range) const noexcept -> U8StringEditor {
    if (range.isEmpty()) {
        return {};
    }
    return U8StringEditor{U8StringSharedStorage{_storage.sharedData(), range}};
}

auto U8StringEditor::dataView() const noexcept -> U8StringDataView {
    return _storage.dataView();
}

}
