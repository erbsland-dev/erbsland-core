// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringEditor.hpp"

#include "U16String.hpp"
#include "U16StringConstIterator.hpp"
#include "U16StringEditorList.hpp"
#include "U16StringList.hpp"
#include "U16StringLiteral.hpp"

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

#include "../impl/escape/StringSafeTransformTools.hpp"
#include "../StringConverter.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>
#include <utility>

namespace erbsland::text {

using namespace impl;
using namespace unit;

auto U16StringEditor::clear() noexcept -> U16StringEditor & {
    _storage.clear();
    return *this;
}

auto U16StringEditor::append(const U16String &text, const ItemCount count) -> U16StringEditor & {
    U16StringAppendTools{_storage}.append(text.dataView(), count);
    return *this;
}

auto U16StringEditor::append(const Char character, CpCount count) -> U16StringEditor & {
    U16StringAppendTools{_storage}.append(character, count);
    return *this;
}

auto U16StringEditor::remove(const U16Range range) -> U16StringEditor & {
    U16StringModifyTools::remove(_storage, range);
    return *this;
}

auto U16StringEditor::remove(const CpRange range) -> U16StringEditor & {
    U16StringModifyTools::remove(_storage, range);
    return *this;
}

auto U16StringEditor::removeAll(const CharSet &characters) -> U16StringEditor & {
    U16StringModifyTools::removeAll(_storage, characters);
    return *this;
}

auto U16StringEditor::removeAll(const U16String &text, const CharCompareFn compareFn) -> U16StringEditor & {
    U16StringModifyTools::remove(_storage, text.dataView(), compareFn);
    return *this;
}

auto U16StringEditor::removeFirst(const U16String &text, const CharCompareFn compareFn) -> U16StringEditor & {
    U16StringModifyTools::removeFirst(_storage, text.dataView(), compareFn);
    return *this;
}

auto U16StringEditor::keep(const U16Range range) -> U16StringEditor & {
    U16StringModifyTools::keep(_storage, range);
    return *this;
}

auto U16StringEditor::keep(const CpRange range) -> U16StringEditor & {
    U16StringModifyTools::keep(_storage, range);
    return *this;
}

auto U16StringEditor::insert(const U16Index index, const U16String &text) -> U16StringEditor & {
    U16StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U16StringEditor::insert(const CpIndex index, const U16String &text) -> U16StringEditor & {
    U16StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U16StringEditor::replace(const U16Range range, const U16String &text) -> U16StringEditor & {
    U16StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U16StringEditor::replace(const CpRange range, const U16String &text) -> U16StringEditor & {
    U16StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U16StringEditor::replaceFirst(const U16String &text, const U16String &replacement, const CharCompareFn compareFn)
    -> U16StringEditor & {
    U16StringModifyTools::replaceFirst(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U16StringEditor::replaceAll(const CharSet &characters, const Char replacement) -> U16StringEditor & {
    U16StringModifyTools::replaceAll(_storage, characters, replacement);
    return *this;
}

auto U16StringEditor::replaceAll(const CharSet &characters, const U16String &replacement) -> U16StringEditor & {
    U16StringModifyTools::replaceAll(_storage, characters, replacement.dataView());
    return *this;
}

auto U16StringEditor::replaceAll(const U16String &text, const U16String &replacement, const CharCompareFn compareFn)
    -> U16StringEditor & {
    U16StringModifyTools::replaceAll(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U16StringEditor::truncate(const CpCount maximumWidth, const TruncateMode mode) -> U16StringEditor & {
    return truncate(maximumWidth, mode, U16String{});
}

auto U16StringEditor::truncate(const CpCount maximumWidth, const TruncateMode mode, const U16String &ellipsis)
    -> U16StringEditor & {
    *this = truncated(maximumWidth, mode, ellipsis);
    return *this;
}

auto U16StringEditor::removed(const U16Range range) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.removedIfChanged(range)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::removed(const CpRange range) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.removedIfChanged(range)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::removedAll(const CharSet &characters) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.removedAllIfChanged(characters)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::removedAll(const U16String &text, const CharCompareFn compareFn) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.removedAllIfChanged(text.dataView(), compareFn)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::removedFirst(const U16String &text, const CharCompareFn compareFn) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.removedFirstIfChanged(text.dataView(), compareFn)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::kept(const U16Range range) const -> U16StringEditor {
    return U16StringEditor{U16StringModifyTools{dataView()}.kept(range)};
}

auto U16StringEditor::kept(const CpRange range) const -> U16StringEditor {
    return U16StringEditor{U16StringModifyTools{dataView()}.kept(range)};
}

auto U16StringEditor::inserted(const U16Index index, const U16String &text) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.insertedIfChanged(index, text.dataView())) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::inserted(const CpIndex index, const U16String &text) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.insertedIfChanged(index, text.dataView())) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::replaced(const U16Range range, const U16String &text) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.replacedIfChanged(range, text.dataView())) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::replaced(const CpRange range, const U16String &text) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.replacedIfChanged(range, text.dataView())) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::replacedFirst(
    const U16String &text, const U16String &replacement, const CharCompareFn compareFn) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.replacedFirstIfChanged(
            text.dataView(), replacement.dataView(), compareFn)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::replacedAll(const CharSet &characters, const Char replacement) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.replacedAllIfChanged(characters, replacement)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::replacedAll(const CharSet &characters, const U16String &replacement) const -> U16StringEditor {
    if (auto result = U16StringModifyTools{dataView()}.replacedAllIfChanged(characters, replacement.dataView())) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::replacedAll(
    const U16String &text, const U16String &replacement, const CharCompareFn compareFn) const -> U16StringEditor {
    if (auto result =
            U16StringModifyTools{dataView()}.replacedAllIfChanged(text.dataView(), replacement.dataView(), compareFn)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::truncated(const CpCount maximumWidth, const TruncateMode mode) const -> U16StringEditor {
    return truncated(maximumWidth, mode, U16String{});
}

auto U16StringEditor::truncated(const CpCount maximumWidth, const TruncateMode mode, const U16String &ellipsis) const
    -> U16StringEditor {
    if (auto result = U16StringTransformTools{dataView()}.truncatedIfChanged(maximumWidth, mode, ellipsis.dataView())) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::aligned(const CpCount length, const geometry::Alignment alignment, const Char fill) const
    -> U16StringEditor {
    if (auto result = U16StringTransformTools{dataView()}.alignedIfChanged(length, alignment, fill)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::toSafeString(const CpCount maximumWidth, const SafeStringFlags flags) const -> U16StringEditor {
    if (auto result =
            StringSafeTransformTools{*this, dataView().dataSpan(), maximumWidth, flags}.toSafeStringIfChanged()) {
        return result.value();
    }
    return *this;
}

void swap(U16StringEditor &first, U16StringEditor &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U16StringEditor::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> U16Count {
    return U16StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U16StringEditor::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U16StringEditor {
    if (auto result = U16StringTransformTools{dataView()}.toEscapedIfChanged(U16String{*this}, format, amount)) {
        return U16StringEditor{std::move(result.value())};
    }
    return *this;
}

auto U16StringEditor::fromCharacter(const Char character, const CpCount count) -> U16StringEditor {
    auto storage = U16StringSharedStorage{};
    U16StringAppendTools{storage}.append(character, count);
    return U16StringEditor{std::move(storage)};
}

auto U16StringEditor::fromJoined(const std::initializer_list<U16String> parts) -> U16StringEditor {
    auto finalSize = std::size_t{0};
    for (const auto &part : parts) {
        finalSize = U16StringSharedStorage::checkedAddSize(finalSize, part.length().toSizeT());
    }
    if (finalSize == 0U) {
        return {};
    }

    auto storage = U16StringSharedStorage::forSize(finalSize);
    auto writePosition = std::size_t{0};
    for (const auto &part : parts) {
        const auto data = part.dataView().dataSpan();
        if (data.empty()) {
            continue;
        }
        std::memcpy(storage.dataForWrite() + writePosition, data.data(), data.size() * sizeof(char16_t));
        writePosition = U16StringSharedStorage::checkedAddSize(writePosition, data.size());
    }
    return U16StringEditor{std::move(storage)};
}

auto U16StringEditor::fromBoolean(const bool value, const BooleanFormat format) -> U16StringEditor {
    return U16StringEditor{StringConverter{format.text(value)}.toU16String()};
}

auto U16StringEditor::fromByteBlock(const mem::ByteBlock &bytes, const ByteFormat format) -> U16StringEditor {
    auto builder = AnyStringBuilder{StringKind::U16};
    builder.appendByteBlock(bytes, format);
    return builder.takeU16StringEditor();
}

auto U16StringEditor::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return U16StringTransformTools{dataView()}.forEach(function);
}

auto U16StringEditor::transformed(const TransformCharacterFn function) const -> U16StringEditor {
    if (auto result = U16StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U16StringEditor{std::move(*result)};
    }
    return *this;
}

void U16StringEditor::reset() noexcept {
    _storage = {};
}

auto U16StringEditor::storageId() const noexcept -> mem::StorageIdentifier {
    return _storage.storageId();
}

void U16StringEditor::reserve(const U16Count capacity) {
    _storage.reserve(capacity);
}

void U16StringEditor::shrinkToFit() {
    _storage.shrinkToFit();
}

auto U16StringEditor::capacity() const noexcept -> U16Count {
    return _storage.capacity();
}

auto U16StringEditor::memoryUsage() const noexcept -> ByteCount {
    return _storage.memoryUsage();
}

void U16StringEditor::detach() {
    _storage.detach();
}

auto U16StringEditor::begin() const noexcept -> const_iterator {
    return U16StringConstIterator{U16String{*this}, indexAt(StringSide::Front)};
}

auto U16StringEditor::end() const noexcept -> const_iterator {
    return U16StringConstIterator{U16String{*this}, indexAt(StringSide::Back)};
}

auto U16StringEditor::withRange(const U16Range range) const noexcept -> U16StringEditor {
    if (range.isEmpty()) {
        return {};
    }
    return U16StringEditor{U16StringSharedStorage{_storage.sharedData(), range}};
}

auto U16StringEditor::dataView() const noexcept -> U16StringDataView {
    return _storage.dataView();
}

auto U16StringEditor::isStorageShared() const noexcept -> bool {
    return _storage.sharedData().isShared();
}

}
