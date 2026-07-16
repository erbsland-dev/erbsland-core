// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16String.hpp"

#include "U16StringCharView.hpp"
#include "U16StringConstIterator.hpp"
#include "U16StringList.hpp"
#include "U16StringLiteral.hpp"
#include "U16StringView.hpp"
#include "U16StringViewList.hpp"

#include "impl/U16Encoding.hpp"
#include "impl/U16StringAppendTools.hpp"
#include "impl/U16StringCharReadTool.hpp"
#include "impl/U16StringComparisonTools.hpp"
#include "impl/U16StringData.hpp"
#include "impl/U16StringEncodingTools.hpp"
#include "impl/U16StringModifyTools.hpp"
#include "impl/U16StringReadTools.hpp"
#include "impl/U16StringTransformTools.hpp"
#include "impl/U16Writer.hpp"

#include "../StringConverter.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32String.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/U8String.hpp"

#include "../../mem/ByteBlockView.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

auto U16String::clear() noexcept -> U16String & {
    _storage.clear();
    return *this;
}

auto U16String::append(const U16StringView &text, const unit::ElementCount count) -> U16String & {
    impl::U16StringAppendTools{_storage}.append(text.dataView(), count);
    return *this;
}

auto U16String::append(const Char character, unit::CpLength count) -> U16String & {
    impl::U16StringAppendTools{_storage}.append(character, count);
    return *this;
}

auto U16String::remove(const unit::U16DataRange range) -> U16String & {
    impl::U16StringModifyTools::remove(_storage, range);
    return *this;
}

auto U16String::remove(const unit::CpRange range) -> U16String & {
    impl::U16StringModifyTools::remove(_storage, range);
    return *this;
}

auto U16String::removeAll(const CharSet &characters) -> U16String & {
    impl::U16StringModifyTools::removeAll(_storage, characters);
    return *this;
}

auto U16String::removeAll(const U16StringView &text, const CharCompareFn compareFn) -> U16String & {
    impl::U16StringModifyTools::remove(_storage, text.dataView(), compareFn);
    return *this;
}

auto U16String::removeFirst(const U16StringView &text, const CharCompareFn compareFn) -> U16String & {
    impl::U16StringModifyTools::removeFirst(_storage, text.dataView(), compareFn);
    return *this;
}

auto U16String::keep(const unit::U16DataRange range) -> U16String & {
    impl::U16StringModifyTools::keep(_storage, range);
    return *this;
}

auto U16String::keep(const unit::CpRange range) -> U16String & {
    impl::U16StringModifyTools::keep(_storage, range);
    return *this;
}

auto U16String::insert(const unit::U16DataIndex index, const U16StringView &text) -> U16String & {
    impl::U16StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U16String::insert(const unit::CpIndex index, const U16StringView &text) -> U16String & {
    impl::U16StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U16String::replace(const unit::U16DataRange range, const U16StringView &text) -> U16String & {
    impl::U16StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U16String::replace(const unit::CpRange range, const U16StringView &text) -> U16String & {
    impl::U16StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U16String::replaceFirst(const U16StringView &text, const U16StringView &replacement, const CharCompareFn compareFn)
    -> U16String & {
    impl::U16StringModifyTools::replaceFirst(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U16String::replaceAll(const CharSet &characters, const Char replacement) -> U16String & {
    impl::U16StringModifyTools::replaceAll(_storage, characters, replacement);
    return *this;
}

auto U16String::replaceAll(const CharSet &characters, const U16StringView &replacement) -> U16String & {
    impl::U16StringModifyTools::replaceAll(_storage, characters, replacement.dataView());
    return *this;
}

auto U16String::replaceAll(const U16StringView &text, const U16StringView &replacement, const CharCompareFn compareFn)
    -> U16String & {
    impl::U16StringModifyTools::replaceAll(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U16String::truncate(const unit::CpLength maximumWidth, const TruncateMode mode) -> U16String & {
    return truncate(maximumWidth, mode, U16StringView{});
}

auto U16String::truncate(const unit::CpLength maximumWidth, const TruncateMode mode, const U16StringView &ellipsis)
    -> U16String & {
    *this = truncated(maximumWidth, mode, ellipsis);
    return *this;
}

auto U16String::removed(const unit::U16DataRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removed(range)};
}

auto U16String::removed(const unit::CpRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removed(range)};
}

auto U16String::removedAll(const CharSet &characters) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removedAll(characters)};
}

auto U16String::removedAll(const U16StringView &text, const CharCompareFn compareFn) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U16String::removedFirst(const U16StringView &text, const CharCompareFn compareFn) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U16String::kept(const unit::U16DataRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.kept(range)};
}

auto U16String::kept(const unit::CpRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.kept(range)};
}

auto U16String::inserted(const unit::U16DataIndex index, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U16String::inserted(const unit::CpIndex index, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U16String::replaced(const unit::U16DataRange range, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U16String::replaced(const unit::CpRange range, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U16String::replacedFirst(
    const U16StringView &text, const U16StringView &replacement, const CharCompareFn compareFn) const -> U16String {
    return U16String{
        impl::U16StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U16String::replacedAll(const CharSet &characters, const Char replacement) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U16String::replacedAll(const CharSet &characters, const U16StringView &replacement) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U16String::replacedAll(
    const U16StringView &text, const U16StringView &replacement, const CharCompareFn compareFn) const -> U16String {
    return U16String{
        impl::U16StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U16String::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U16String {
    return truncated(maximumWidth, mode, U16StringView{});
}

auto U16String::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U16StringView &ellipsis) const -> U16String {
    return U16String{impl::U16StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U16String::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U16String {
    return U16String{impl::U16StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U16String::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const -> U16String {
    return impl::U16StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

void swap(U16String &first, U16String &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U16String::toCharView() const noexcept -> U16StringCharView {
    return U16StringCharView{_storage};
}

auto U16String::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept
    -> unit::U16DataLength {
    return impl::U16StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U16String::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U16String {
    return impl::U16StringTransformTools{dataView()}.toEscaped(format, amount);
}

auto U16String::fromCharacter(const Char character, const unit::CpLength count) -> U16String {
    auto storage = impl::U16StringSharedStorage{};
    impl::U16StringAppendTools{storage}.append(character, count);
    return U16String{std::move(storage)};
}

auto U16String::fromJoined(const std::initializer_list<U16StringView> parts) -> U16String {
    auto finalSize = std::size_t{0};
    for (const auto &part : parts) {
        finalSize = impl::U16StringSharedStorage::checkedAddSize(
            finalSize, part.length().toSizeT(), "Joined string exceeds size bounds");
    }
    if (finalSize == 0U) {
        return {};
    }

    auto storage = impl::U16StringSharedStorage::forSize(finalSize);
    auto writePosition = std::size_t{0};
    for (const auto &part : parts) {
        const auto data = part.dataView().dataSpan();
        if (data.empty()) {
            continue;
        }
        std::memcpy(storage.dataForWrite() + writePosition, data.data(), data.size() * sizeof(char16_t));
        writePosition = impl::U16StringSharedStorage::checkedAddSize(
            writePosition, data.size(), "Joined string write exceeds size bounds");
    }
    return U16String{std::move(storage)};
}

auto U16String::fromBoolean(const bool value, const BooleanFormat format) -> U16String {
    return StringConverter{format.text(value)}.toU16String();
}

auto U16String::fromByteBlock(const mem::ByteBlockView &bytes, const ByteFormat format) -> U16String {
    auto builder = StringBuilder{StringKind::U16};
    builder.appendByteBlock(bytes, format);
    return builder.takeU16String();
}

auto U16String::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U16StringTransformTools{dataView()}.forEach(function);
}

auto U16String::transformed(const TransformCharacterFn function) const -> U16String {
    if (auto result = impl::U16StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U16String{std::move(*result)};
    }
    return *this;
}

void U16String::reset() noexcept {
    _storage = {};
}

auto U16String::storageId() const noexcept -> mem::StorageIdentifier {
    return _storage.storageId();
}

void U16String::reserve(const unit::U16DataLength capacity) {
    _storage.reserve(capacity);
}

void U16String::shrinkToFit() {
    _storage.shrinkToFit();
}

auto U16String::capacity() const noexcept -> unit::U16DataLength {
    return _storage.capacity();
}

auto U16String::memoryUsage() const noexcept -> unit::ByteLength {
    return _storage.memoryUsage();
}

void U16String::detach() {
    _storage.detach();
}

auto U16String::begin() const noexcept -> const_iterator {
    return U16StringConstIterator{U16StringView{*this}, indexAt(StringSide::Front)};
}

auto U16String::end() const noexcept -> const_iterator {
    return U16StringConstIterator{U16StringView{*this}, indexAt(StringSide::Back)};
}

auto U16String::withRange(const unit::U16DataRange range) const noexcept -> U16String {
    if (range.isEmpty()) {
        return {};
    }
    return U16String{impl::U16StringSharedStorage{_storage.sharedData(), range}};
}

auto U16String::dataView() const noexcept -> impl::U16StringDataView {
    return _storage.dataView();
}

}
