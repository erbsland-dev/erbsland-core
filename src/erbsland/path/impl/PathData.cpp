// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathData.hpp"

#include "BackendFactory.hpp"
#include "CharactersSets.hpp"
#include "PathConstants.hpp"
#include "PathInfoCache.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/CpIndex.hpp"

#include <memory>
#include <type_traits>

namespace erbsland::path::impl {

using namespace text::literals;
using namespace text;
using namespace unit;

PathData::PathData(
    const PathFormat format, text::String root, text::StringList elements, const unit::CpLength characterLength) :
    _format{format}, _root{std::move(root)}, _elements{std::move(elements)}, _characterLength{characterLength} {
}

PathData::PathData(const PathData &other) :
    mem::SharedData{},
    _format{other._format},
    _root{other._root},
    _elements{other._elements},
    _characterLength{other._characterLength} {
}

PathData::PathData(PathData &&other) noexcept :
    mem::SharedData{},
    _format{other._format},
    _root{std::move(other._root)},
    _elements{std::move(other._elements)},
    _characterLength{other._characterLength},
    _infoCache{other._infoCache.exchange(nullptr)} {
}

PathData::~PathData() {
    resetInfoCache();
}

auto PathData::operator=(const PathData &other) -> PathData & {
    if (this != &other) {
        _format = other._format;
        _root = other._root;
        _elements = other._elements;
        _characterLength = other._characterLength;
        resetInfoCache();
    }
    return *this;
}

auto PathData::operator=(PathData &&other) noexcept -> PathData & {
    if (this != &other) {
        _format = other._format;
        _root = std::move(other._root);
        _elements = std::move(other._elements);
        _characterLength = other._characterLength;
        auto *movedCache = other._infoCache.exchange(nullptr);
        delete _infoCache.exchange(movedCache);
    }
    return *this;
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

auto PathData::root() const noexcept -> text::String {
    return _root;
}

auto PathData::elements() const noexcept -> text::StringList {
    return _elements;
}

void PathData::setFormat(PathFormat format) noexcept {
    _format = format;
    resetInfoCache();
}

void PathData::setRoot(text::String root) noexcept {
    _root = std::move(root);
    updateCharacterLength();
    resetInfoCache();
}

void PathData::setElements(text::StringList elements) noexcept {
    _elements = std::move(elements);
    updateCharacterLength();
    resetInfoCache();
}

auto PathData::publicItemCount() const noexcept -> unit::ItemCount {
    auto result = _elements.count();
    if (!_root.isEmpty()) {
        ++result;
    }
    return result;
}

auto PathData::publicElements() const -> text::StringList {
    auto result = StringList{};
    result.reserve(publicItemCount());
    if (!_root.isEmpty()) {
        result.append(_root);
    }
    result.append(_elements);
    return result;
}

auto PathData::infoCache() const -> PathInfoCache * {
    auto *result = _infoCache.load();
    if (result != nullptr) {
        return result;
    }
    auto *created = new PathInfoCache{};
    if (_infoCache.compare_exchange_strong(result, created)) {
        return created;
    }
    delete created;
    return result;
}

void PathData::resetInfoCache() noexcept {
    delete _infoCache.exchange(nullptr);
}

void PathData::updateCharacterLength() noexcept {
    _characterLength = _root.characterLength();
    auto first = true;
    for (const auto &element : _elements) {
        if (!first) {
            _characterLength += CpLength::one();
        }
        _characterLength += element.characterLength();
        first = false;
    }
}

auto PathData::toString() const -> String {
    auto length = _root.length();
    auto needsSeparator = !_root.isEmpty() && !_root.endsWith(cSlash);
    for (const auto &element : _elements) {
        if (needsSeparator) {
            length += cSlash.length();
        }
        length += element.length();
        needsSeparator = true;
    }

    auto result = StringEditor{};
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
    return result;
}

auto PathData::toWindows(const PathWindowsFormat format) const -> String {
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

    auto result = StringEditor{};
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
    return result;
}

template <typename tData>
auto PathData::create(const PathFormat format, const String &root, StringList elements) noexcept
    -> mem::SharedDataPointer<tData> {
    static_assert(std::is_same_v<tData, PathData>);

    auto publicItemCount = elements.count();
    if (!root.isEmpty()) {
        publicItemCount += ItemCount::one();
    }
    if (publicItemCount.isZero() || publicItemCount > cMaximumPathElements) {
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
    return PathDataPtr{new PathData{format, root, std::move(elements), totalLength}};
}

template auto PathData::create<PathData>(PathFormat, const String &, StringList) noexcept -> PathDataPtr;

auto PathData::createJoined(const PathData &base, const String &element) noexcept -> std::unique_ptr<PathData> {
    if (element.isEmpty() || element.containsOneOf(invalidPathCharacters())) {
        return {};
    }
    const auto elementCount = base.publicItemCount() + ItemCount::one();
    auto characterLength = base._characterLength + element.characterLength();
    if (!base._elements.isEmpty()) {
        characterLength += CpLength::one();
    }
    if (elementCount > cMaximumPathElements || characterLength > cMaximumPathCharacters) {
        return {};
    }
    auto elements = base._elements;
    elements.append(element);
    return std::make_unique<PathData>(base._format, base._root, std::move(elements), characterLength);
}

auto PathData::nonRootElementsFromPublicSlice(const StringList &elements, bool &sliceStartsWithRoot) -> StringList {
    sliceStartsWithRoot = false;
    auto result = StringList{};
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

void PathData::appendWindowsRoot(StringEditor &result, const PathWindowsFormat format) const {
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

void PathData::appendRootWithWindowsSeparators(
    StringEditor &result, const String &root, const bool skipFirstCharacter) {
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
