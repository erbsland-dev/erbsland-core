// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8String.hpp"

#include "U8StringCharView.hpp"
#include "U8StringConstIterator.hpp"
#include "U8StringList.hpp"
#include "U8StringLiteral.hpp"
#include "U8StringView.hpp"
#include "U8StringViewList.hpp"

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
#include "../u16/U16String.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32String.hpp"

#include "../../mem/ByteBlockView.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

auto U8String::clear() noexcept -> U8String & {
    _storage.clear();
    return *this;
}

auto U8String::append(const U8StringView &text, const unit::ElementCount count) -> U8String & {
    impl::U8StringAppendTools{_storage}.append(text.dataView(), count);
    return *this;
}

auto U8String::append(const Char character, unit::CpLength count) -> U8String & {
    impl::U8StringAppendTools{_storage}.append(character, count);
    return *this;
}

auto U8String::remove(const unit::ByteRange range) -> U8String & {
    impl::U8StringModifyTools::remove(_storage, range);
    return *this;
}

auto U8String::remove(const unit::CpRange range) -> U8String & {
    impl::U8StringModifyTools::remove(_storage, range);
    return *this;
}

auto U8String::removeAll(const CharSet &characters) -> U8String & {
    impl::U8StringModifyTools::removeAll(_storage, characters);
    return *this;
}

auto U8String::removeAll(const U8StringView &text, const CharCompareFn compareFn) -> U8String & {
    impl::U8StringModifyTools::remove(_storage, text.dataView(), compareFn);
    return *this;
}

auto U8String::removeFirst(const U8StringView &text, const CharCompareFn compareFn) -> U8String & {
    impl::U8StringModifyTools::removeFirst(_storage, text.dataView(), compareFn);
    return *this;
}

auto U8String::keep(const unit::ByteRange range) -> U8String & {
    impl::U8StringModifyTools::keep(_storage, range);
    return *this;
}

auto U8String::keep(const unit::CpRange range) -> U8String & {
    impl::U8StringModifyTools::keep(_storage, range);
    return *this;
}

auto U8String::insert(const unit::ByteIndex index, const U8StringView &text) -> U8String & {
    impl::U8StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U8String::insert(const unit::CpIndex index, const U8StringView &text) -> U8String & {
    impl::U8StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U8String::replace(const unit::ByteRange range, const U8StringView &text) -> U8String & {
    impl::U8StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U8String::replace(const unit::CpRange range, const U8StringView &text) -> U8String & {
    impl::U8StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U8String::replaceFirst(const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn)
    -> U8String & {
    impl::U8StringModifyTools::replaceFirst(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U8String::replaceAll(const CharSet &characters, const Char replacement) -> U8String & {
    impl::U8StringModifyTools::replaceAll(_storage, characters, replacement);
    return *this;
}

auto U8String::replaceAll(const CharSet &characters, const U8StringView &replacement) -> U8String & {
    impl::U8StringModifyTools::replaceAll(_storage, characters, replacement.dataView());
    return *this;
}

auto U8String::replaceAll(const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn)
    -> U8String & {
    impl::U8StringModifyTools::replaceAll(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U8String::truncate(const unit::CpLength maximumWidth, const TruncateMode mode) -> U8String & {
    return truncate(maximumWidth, mode, U8StringView{});
}

auto U8String::truncate(const unit::CpLength maximumWidth, const TruncateMode mode, const U8StringView &ellipsis)
    -> U8String & {
    *this = truncated(maximumWidth, mode, ellipsis);
    return *this;
}

auto U8String::removed(const unit::ByteRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(range)};
}

auto U8String::removed(const unit::CpRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(range)};
}

auto U8String::removedAll(const CharSet &characters) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removedAll(characters)};
}

auto U8String::removedAll(const U8StringView &text, const CharCompareFn compareFn) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U8String::removedFirst(const U8StringView &text, const CharCompareFn compareFn) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U8String::kept(const unit::ByteRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.kept(range)};
}

auto U8String::kept(const unit::CpRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.kept(range)};
}

auto U8String::inserted(const unit::ByteIndex index, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8String::inserted(const unit::CpIndex index, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8String::replaced(const unit::ByteRange range, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8String::replaced(const unit::CpRange range, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8String::replacedFirst(
    const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn) const -> U8String {
    return U8String{
        impl::U8StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8String::replacedAll(const CharSet &characters, const Char replacement) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U8String::replacedAll(const CharSet &characters, const U8StringView &replacement) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U8String::replacedAll(
    const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn) const -> U8String {
    return U8String{
        impl::U8StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8String::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U8String {
    return truncated(maximumWidth, mode, U8StringView{});
}

auto U8String::truncated(const unit::CpLength maximumWidth, const TruncateMode mode, const U8StringView &ellipsis) const
    -> U8String {
    return U8String{impl::U8StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U8String::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U8String {
    return U8String{impl::U8StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U8String::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const -> U8String {
    return impl::U8StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

void swap(U8String &first, U8String &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U8String::toCharView() const noexcept -> U8StringCharView {
    return U8StringCharView{_storage};
}

void U8String::reset() noexcept {
    _storage = {};
}

auto U8String::storageId() const noexcept -> mem::StorageIdentifier {
    return _storage.storageId();
}

void U8String::reserve(const unit::ByteLength capacity) {
    _storage.reserve(capacity);
}

void U8String::shrinkToFit() {
    _storage.shrinkToFit();
}

auto U8String::capacity() const noexcept -> unit::ByteLength {
    return _storage.capacity();
}

auto U8String::memoryUsage() const noexcept -> unit::ByteLength {
    return _storage.memoryUsage();
}

auto U8String::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> unit::ByteLength {
    return impl::U8StringTransformTools{_storage.dataView()}.escapedSize(format, amount);
}

auto U8String::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U8String {
    return impl::U8StringTransformTools{_storage.dataView()}.toEscaped(format, amount);
}

auto U8String::fromCharacter(const Char character, const unit::CpLength count) -> U8String {
    auto storage = impl::U8StringSharedStorage{};
    impl::U8StringAppendTools{storage}.append(character, count);
    return U8String{std::move(storage)};
}

auto U8String::fromJoined(const std::initializer_list<U8StringView> parts) -> U8String {
    auto finalSize = std::size_t{0};
    for (const auto &part : parts) {
        finalSize = impl::U8StringSharedStorage::checkedAddSize(
            finalSize, part.length().toSizeT(), "Joined string exceeds size bounds");
    }
    if (finalSize == 0U) {
        return {};
    }

    auto storage = impl::U8StringSharedStorage::forSize(finalSize);
    auto writePosition = std::size_t{0};
    for (const auto &part : parts) {
        const auto data = part.dataView().dataSpan();
        if (data.empty()) {
            continue;
        }
        std::memcpy(storage.dataForWrite() + writePosition, data.data(), data.size());
        writePosition = impl::U8StringSharedStorage::checkedAddSize(
            writePosition, data.size(), "Joined string write exceeds size bounds");
    }
    return U8String{std::move(storage)};
}

auto U8String::fromBoolean(const bool value, const BooleanFormat format) -> U8String {
    return U8String{format.text(value)};
}

auto U8String::fromByteBlock(const mem::ByteBlockView &bytes, const ByteFormat format) -> U8String {
    auto builder = StringBuilder{};
    builder.appendByteBlock(bytes, format);
    return builder.takeU8String();
}

auto U8String::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.toRawValue());
        return true;
    });
    return result;
}

auto U8String::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.caseFolded().toRawValue());
        return true;
    });
    return result;
}

auto U8String::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U8StringTransformTools{dataView()}.forEach(function);
}

auto U8String::transformed(const TransformCharacterFn function) const -> U8String {
    if (auto result = impl::U8StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U8String{std::move(*result)};
    }
    return *this;
}

void U8String::detach() {
    _storage.detach();
}

auto U8String::begin() const noexcept -> const_iterator {
    return U8StringConstIterator{U8StringView{*this}, indexAt(StringSide::Front)};
}

auto U8String::end() const noexcept -> const_iterator {
    return U8StringConstIterator{U8StringView{*this}, indexAt(StringSide::Back)};
}

auto U8String::withRange(const unit::ByteRange range) const noexcept -> U8String {
    if (range.isEmpty()) {
        return {};
    }
    return U8String{impl::U8StringSharedStorage{_storage.sharedData(), range}};
}

auto U8String::dataView() const noexcept -> impl::U8StringDataView {
    return _storage.dataView();
}

}
