// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Path.hpp"

#include "PathContent.hpp"
#include "PathError.hpp"
#include "PathInfo.hpp"
#include "PathOperations.hpp"
#include "PathWalker.hpp"

#include "impl/BackendFactory.hpp"
#include "impl/PathBackend.hpp"
#include "impl/PathNameTools.hpp"
#include "impl/PathParser.hpp"

#include "../core/Definitions.hpp"
#include "../err/ParseError.hpp"
#include "../text/StringConverter.hpp"
#include "../text/StringEditor.hpp"

#include <utility>

namespace erbsland::path {
using namespace text::literals;
using impl::PathData;
using impl::PathDataPtr;
using impl::PathParseMode;
using impl::PathParser;
using text::CharCompareFn;
using text::String;
using text::StringConverter;
using text::StringList;
using unit::ItemCount;
using unit::ItemIndex;
using unit::ItemRange;

Path::Path(const String &path) noexcept {
    try {
        _data = PathParser{path, PathParseMode::Generic}.parse();
    } catch (const err::ParseError &) {}
}

Path::Path(const std::filesystem::path &path) noexcept {
    try {
        _data = PathParser{String{path.generic_string()}, PathParseMode::Generic}.parse();
    } catch (const err::ParseError &) {}
}

Path::Path(PathDataPtr data) noexcept : _data{std::move(data)} {
}

auto Path::operator/(const Path &other) const -> Path {
    return joined(other);
}

auto Path::operator/(const String &other) const -> Path {
    return joined(other);
}

auto Path::operator/=(const Path &other) -> Path & {
    return join(other);
}

auto Path::operator/=(const String &other) -> Path & {
    return join(other);
}

auto Path::isEmpty() const noexcept -> bool {
    return _data.isNull();
}

auto Path::isValid() const noexcept -> bool {
    return !isEmpty();
}

auto Path::isRelative() const noexcept -> bool {
    return !isEmpty() && !_data->isAbsolute();
}

auto Path::isAbsolute() const noexcept -> bool {
    return !isEmpty() && _data->isAbsolute();
}

auto Path::isRoot() const noexcept -> bool {
    return !isEmpty() && _data->isRoot();
}

auto Path::compare(const Path &other, const CharCompareFn compareFn) const noexcept -> std::strong_ordering {
    if (isEmpty() && other.isEmpty()) {
        return std::strong_ordering::equal;
    }
    if (isEmpty()) {
        return std::strong_ordering::less;
    }
    if (other.isEmpty()) {
        return std::strong_ordering::greater;
    }
    return toString().compare(other.toString(), compareFn);
}

auto Path::format() const noexcept -> PathFormat {
    return isEmpty() ? PathFormat::Generic : _data->format();
}

auto Path::elementCount() const noexcept -> ItemCount {
    return isEmpty() ? ItemCount::zero() : _data->publicItemCount();
}

auto Path::element(const ItemIndex index) const noexcept -> String {
    if (isEmpty() || !index.isValid() || !index.isWithin(elementCount())) {
        return {};
    }
    if (_data->isAbsolute()) {
        if (index.isZero()) {
            return _data->root();
        }
        return _data->elements().get(index - ItemCount::one());
    }
    return _data->elements().get(index);
}

auto Path::elements() const noexcept -> StringList {
    return isEmpty() ? StringList{} : _data->publicElements();
}

auto Path::parent() const noexcept -> Path {
    if (isEmpty() || isRoot()) {
        return {};
    }
    if (_data->elements().count().isOne()) {
        if (_data->isAbsolute()) {
            return Path{PathData::create(_data->format(), _data->root(), {})};
        }
        return {};
    }
    return Path{PathData::create(
        _data->format(), _data->root(), _data->elements().prefix(_data->elements().count() - ItemCount::one()))};
}

auto Path::parents() const noexcept -> PathList {
    auto result = PathList{};
    auto current = parent();
    while (!current.isEmpty()) {
        result.append(current);
        if (current.isRoot()) {
            break;
        }
        current = current.parent();
    }
    return result;
}

auto Path::root() const noexcept -> String {
    return isEmpty() ? String{} : _data->root();
}

auto Path::name() const noexcept -> String {
    if (isEmpty() || _data->elements().count().isZero()) {
        return {};
    }
    return _data->elements().last();
}

auto Path::suffix() const noexcept -> String {
    return impl::lastSuffix(name());
}

auto Path::suffixes() const noexcept -> String {
    return impl::suffixes(name());
}

auto Path::stem() const noexcept -> String {
    return impl::stem(name());
}

auto Path::withName(const String &newName) const noexcept -> Path {
    if (isEmpty() || newName.isEmpty() || !newName.isValidUtf8()) {
        return {};
    }
    auto elements = _data->elements();
    if (elements.count().isZero()) {
        return {};
    }
    elements.set(ItemIndex::zero() + (elements.count() - ItemCount::one()), newName);
    return Path{PathData::create(_data->format(), _data->root(), std::move(elements))};
}

auto Path::withSuffix(const String &replacement) const noexcept -> Path {
    if (isEmpty() || !replacement.isValidUtf8()) {
        return {};
    }
    return withName(String::fromJoined({stem(), impl::normalizedSuffixReplacement(replacement)}));
}

auto Path::withStem(const String &replacement) const noexcept -> Path {
    if (isEmpty() || replacement.isEmpty() || !replacement.isValidUtf8()) {
        return {};
    }
    return withName(String::fromJoined({replacement, suffixes()}));
}

auto Path::join(const Path &other) noexcept -> Path & {
    if (isEmpty() || other.isEmpty()) {
        return *this;
    }
    auto elements = _data->elements();
    elements.append(other._data->elements());
    _data = PathData::create(_data->format(), _data->root(), std::move(elements));
    return *this;
}

auto Path::join(const String &other) noexcept -> Path & {
    return join(Path{other});
}

auto Path::joined(const Path &other) const noexcept -> Path {
    auto result = *this;
    result.join(other);
    return result;
}

auto Path::joined(const String &other) const noexcept -> Path {
    return joined(Path{other});
}

auto Path::slice(const ItemRange range) const noexcept -> Path {
    if (isEmpty()) {
        return {};
    }
    const auto clampedRange = range.clampedTo(elementCount());
    auto slicedElements = elements().slice(clampedRange);
    if (slicedElements.count().isZero()) {
        return {};
    }
    auto startsWithRoot = false;
    auto nonRootElements = PathData::nonRootElementsFromPublicSlice(slicedElements, startsWithRoot);
    return Path{PathData::create(
        startsWithRoot ? _data->format() : PathFormat::Generic,
        startsWithRoot ? slicedElements.first() : String{},
        std::move(nonRootElements))};
}

auto Path::splitAfter(const ItemCount count) const noexcept -> std::pair<Path, Path> {
    if (isEmpty()) {
        return {{}, {}};
    }
    if (count.isZero()) {
        return {{}, *this};
    }
    if (count.isInfinite() || count >= elementCount()) {
        return {*this, currentElement()};
    }
    return {
        slice({ItemIndex::zero(), count}),
        slice({ItemIndex::end(count), ItemCount::infinite()}),
    };
}

auto Path::resolve(const PathResolveOptions options) const noexcept -> Path {
    try {
        return resolveOrThrow(options);
    } catch (const PathError &) {
        return {};
    }
}

auto Path::resolveOrThrow(const PathResolveOptions options) const -> Path {
    return impl::pathBackend().resolveOrThrow(*this, options);
}

auto Path::toAbsolute(std::optional<Path> base) const noexcept -> Path {
    try {
        return toAbsoluteOrThrow(std::move(base));
    } catch (const PathError &) {
        return {};
    }
}

auto Path::toAbsoluteOrThrow(std::optional<Path> base) const -> Path {
    return impl::pathBackend().toAbsoluteOrThrow(*this, std::move(base));
}

auto Path::toRelative(std::optional<Path> base) const noexcept -> Path {
    try {
        return toRelativeOrThrow(std::move(base));
    } catch (const PathError &) {
        return {};
    }
}

auto Path::toRelativeOrThrow(std::optional<Path> base) const -> Path {
    return impl::pathBackend().toRelativeOrThrow(*this, std::move(base));
}

auto Path::isRelativeTo(std::optional<Path> base) const noexcept -> bool {
    return impl::pathBackend().isRelativeTo(*this, std::move(base));
}

auto Path::commonAncestor(std::optional<Path> base) const noexcept -> Path {
    return impl::pathBackend().commonAncestor(*this, std::move(base));
}

auto Path::info(const PathInfoParts parts) const noexcept -> PathInfo {
    return PathInfo{*this, parts};
}

auto Path::walker() const noexcept -> PathWalker {
    return PathWalker{*this};
}

auto Path::content() const -> PathContent {
    return PathContent{*this};
}

auto Path::operations() const noexcept -> PathOperations {
    return PathOperations{*this};
}

auto Path::toStdPath() const noexcept -> std::filesystem::path {
#ifdef ERBSLAND_OS_WINDOWS
    return std::filesystem::path{StringConverter{toWindows()}.toStdWString()};
#else
    return std::filesystem::path{StringConverter{toString()}.toStdString()};
#endif
}

auto Path::toPosix() const noexcept -> String {
    if (isEmpty() || (format() == PathFormat::Windows && isAbsolute())) {
        return {};
    }
    return toString();
}

auto Path::toWindows(const PathWindowsFormat format) const noexcept -> String {
    if (isEmpty()) {
        return {};
    }
    return _data->toWindows(format);
}

auto Path::toString() const noexcept -> String {
    return isEmpty() ? String{} : _data->toString();
}

auto Path::fromElements(const StringList &elements) noexcept -> Path {
    if (elements.count().isZero()) {
        return {};
    }
    auto startsWithRoot = false;
    auto nonRootElements = PathData::nonRootElementsFromPublicSlice(elements, startsWithRoot);
    return Path{PathData::create(
        PathFormat::Generic, startsWithRoot ? elements.first() : String{}, std::move(nonRootElements))};
}

auto Path::fromPosix(const String &path) noexcept -> Path {
    try {
        return Path{PathParser{path, PathParseMode::Posix}.parse()};
    } catch (const err::ParseError &) {
        return {};
    }
}

auto Path::fromPosixOrThrow(const String &path) -> Path {
    return Path{PathParser{path, PathParseMode::Posix}.parse()};
}

auto Path::fromWindows(const String &path) noexcept -> Path {
    try {
        return Path{PathParser{path, PathParseMode::Windows}.parse()};
    } catch (const err::ParseError &) {
        return {};
    }
}

auto Path::fromWindowsOrThrow(const String &path) -> Path {
    return Path{PathParser{path, PathParseMode::Windows}.parse()};
}

auto Path::fromNative(const String &path) noexcept -> Path {
    try {
        return Path{PathParser{path, PathParseMode::Native}.parse()};
    } catch (const err::ParseError &) {
        return {};
    }
}

auto Path::fromNativeOrThrow(const String &path) -> Path {
    return Path{PathParser{path, PathParseMode::Native}.parse()};
}

auto Path::empty() noexcept -> const Path & {
    static const auto path = Path{};
    return path;
}

auto Path::currentDirectory() noexcept -> Path {
    try {
        return impl::pathBackend().currentDirectoryOrThrow();
    } catch (const PathError &) {
        return {};
    }
}

auto Path::userHomeDirectory() noexcept -> Path {
    try {
        return userHomeDirectoryOrThrow();
    } catch (const PathError &) {
        return {};
    }
}

auto Path::userHomeDirectoryOrThrow() -> Path {
    return impl::pathBackend().userHomeDirectoryOrThrow();
}

auto Path::systemTempDirectory() noexcept -> Path {
    try {
        return systemTempDirectoryOrThrow();
    } catch (const PathError &) {
        return {};
    }
}

auto Path::systemTempDirectoryOrThrow() -> Path {
    return impl::pathBackend().systemTempDirectoryOrThrow();
}

auto Path::currentElement() noexcept -> Path {
    return Path{PathData::create(PathFormat::Generic, {}, StringList{{"."_el}})};
}

auto Path::parentElement() noexcept -> Path {
    return Path{PathData::create(PathFormat::Generic, {}, StringList{{".."_el}})};
}

}
