// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathData.hpp"

#include "BackendFactory.hpp"
#include "CharactersSets.hpp"
#include "PathConstants.hpp"

#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../unit/CpIndex.hpp"

#include <type_traits>

namespace erbsland::path::impl {

using namespace text::literals;
using namespace text;
using namespace unit;

PathData::PathData(const PathFormat format, text::StringView root, text::StringViewList elements) :
    _format{format}, _root{std::move(root)}, _elements{std::move(elements)} {
}

auto PathData::isAbsolute() const noexcept -> bool {
    return !_root.isEmpty();
}

auto PathData::isRoot() const noexcept -> bool {
    return isAbsolute() && _elements.count().isZero();
}

auto PathData::format() const noexcept -> PathFormat {
    return _format;
}

auto PathData::root() const noexcept -> text::StringView {
    return _root;
}

auto PathData::elements() const noexcept -> text::StringViewList {
    return _elements;
}

void PathData::setFormat(PathFormat format) noexcept {
    _format = format;
}

void PathData::setRoot(text::StringView root) noexcept {
    _root = std::move(root);
}

void PathData::setElements(text::StringViewList elements) noexcept {
    _elements = std::move(elements);
}

auto PathData::publicElementCount() const noexcept -> unit::ElementCount {
    auto result = _elements.count();
    if (!_root.isEmpty()) {
        ++result;
    }
    return result;
}

auto PathData::publicElements() const -> text::StringViewList {
    auto result = StringViewList{};
    result.reserve(publicElementCount());
    if (!_root.isEmpty()) {
        result.append(_root);
    }
    result.append(_elements);
    return result;
}

auto PathData::toString() const -> StringView {
    auto length = _root.length();
    auto needsSeparator = !_root.isEmpty() && !_root.endsWith(cSlash);
    for (const auto &element : _elements) {
        if (needsSeparator) {
            length += cSlash.length();
        }
        length += element.length();
        needsSeparator = true;
    }

    auto result = String{};
    result.reserve(length);
    if (!_root.isEmpty()) {
        result.append(_root);
    }
    needsSeparator = !_root.isEmpty() && !_root.endsWith(cSlash);
    for (const auto &element : _elements) {
        if (needsSeparator) {
            result.append(cSlash);
        }
        result.append(element);
        needsSeparator = true;
    }
    return StringView{result};
}

auto PathData::toWindows(const PathWindowsFormat format) const -> StringView {
    if (_format == PathFormat::Posix && isAbsolute()) {
        return {};
    }

    auto length = windowsRootLength(format);
    auto first = true;
    for (const auto &element : _elements) {
        if (!first) {
            length += cWindowsSeparator.length();
        }
        length += element.length();
        first = false;
    }

    auto result = String{};
    result.reserve(length);
    appendWindowsRoot(result, format);
    first = true;
    for (const auto &element : _elements) {
        if (!first) {
            result.append(cWindowsSeparator);
        }
        result.append(element);
        first = false;
    }
    return StringView{result};
}

template <typename tData>
auto PathData::create(const PathFormat format, const StringView &root, StringViewList elements) noexcept
    -> mem::SharedDataPointer<tData> {
    static_assert(std::is_same_v<tData, PathData>);

    auto publicElementCount = elements.count();
    if (!root.isEmpty()) {
        publicElementCount += ElementCount::one();
    }
    if (publicElementCount.isZero() || publicElementCount > cMaximumPathElements) {
        return {};
    }
    CpLength totalLength = root.characterLength();
    if (!root.isEmpty() && root.containsOneOf(invalidPathCharacters())) {
        return {};
    }
    bool first = true;
    for (const auto &element : elements) {
        totalLength += element.characterLength();
        if (first) {
            first = false;
        } else {
            totalLength += CpLength::one();
        }
        if (element.containsOneOf(invalidPathCharacters())) {
            return {};
        }
    }
    if (totalLength > cMaximumPathCharacters) {
        return {};
    }
    return PathDataPtr{new PathData{format, root, std::move(elements)}};
}

template auto PathData::create<PathData>(PathFormat, const StringView &, StringViewList) noexcept -> PathDataPtr;

auto PathData::nonRootElementsFromPublicSlice(const StringViewList &elements, bool &sliceStartsWithRoot)
    -> StringViewList {
    sliceStartsWithRoot = false;
    auto result = StringViewList{};
    result.reserve(elements.count());
    auto first = true;
    for (const auto &element : elements) {
        if (first && (element == cSlash || element.endsWith(cDriveRootSuffix) || element.startsWith(cDoubleSlash))) {
            sliceStartsWithRoot = true;
        } else {
            result.append(element);
        }
        first = false;
    }
    return result;
}

auto PathData::backend() noexcept -> PathBackend & {
    return pathBackend();
}

auto PathData::windowsRootLength(const PathWindowsFormat format) const noexcept -> ByteLength {
    if (_root.isEmpty() || _root == cSlash) {
        return {};
    }
    if (_root.startsWith(cDoubleSlash)) {
        if (format == PathWindowsFormat::Extended) {
            return cWindowsExtendedNativeUncPathPrefix.length() + _root.length() - ByteLength::one();
        }
        return _root.length();
    }
    if (format == PathWindowsFormat::Extended) {
        return cWindowsExtendedNativePathPrefix.length() + _root.length();
    }
    return _root.length();
}

void PathData::appendWindowsRoot(String &result, const PathWindowsFormat format) const {
    if (_root.isEmpty() || _root == cSlash) {
        return;
    }
    if (_root.startsWith(cDoubleSlash)) {
        if (format == PathWindowsFormat::Extended) {
            result.append(cWindowsExtendedNativeUncPathPrefix);
            appendRootWithWindowsSeparators(result, _root, true);
            return;
        }
        appendRootWithWindowsSeparators(result, _root, false);
        return;
    }

    if (format == PathWindowsFormat::Extended) {
        result.append(cWindowsExtendedNativePathPrefix);
    }
    const auto drive = _root.splitAt(CpIndex{2U}).first;
    result.append(drive);
    result.append(cWindowsSeparator);
}

void PathData::appendRootWithWindowsSeparators(String &result, const StringView &root, const bool skipFirstCharacter) {
    auto reader = StringCharReader{root};
    auto isFirstCharacter = true;
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (skipFirstCharacter && isFirstCharacter) {
            isFirstCharacter = false;
            continue;
        }
        isFirstCharacter = false;
        result.append(character == U'/' ? Char{U'\\'} : character);
    }
}

}
