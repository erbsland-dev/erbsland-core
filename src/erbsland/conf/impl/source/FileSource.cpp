// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileSource.hpp"

#include "../../../path/PathContent.hpp"
#include "../../../path/PathInfo.hpp"
#include "../../../path/PathReadTextOptions.hpp"
#include "../../../text/StringBomMode.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../../unit/LineCount.hpp"
#include "../../ConfError.hpp"

#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

FileSource::FileSource(path::Path path) noexcept :
    _path{std::move(path)}, _identifier{SourceIdentifier::createForFile(_path.toAbsolute().toString())} {
}

auto FileSource::identifier() const noexcept -> SourceIdentifierPtr {
    return _identifier;
}

auto FileSource::codeSnippet(const unit::CodeLocation location) noexcept -> std::optional<text::CodeSnippet> {
    if (auto result = TextStreamSource::codeSnippet(location); result.has_value()) {
        return result;
    }
    static constexpr auto cMaximumSnippetLine = std::size_t{10'000U};
    static constexpr auto cMaximumSnippetFileSize = std::size_t{1'000'000U};
    if (location.line().isNoIndex() || location.line().toSizeT() >= cMaximumSnippetLine) {
        return std::nullopt;
    }
    try {
        const auto info = _path.info();
        if (!info.isRegularFile() || info.fileSize() > unit::ByteLength::fromSizeT(cMaximumSnippetFileSize)) {
            return std::nullopt;
        }
        auto source = FileSource{info.resolvedPath()};
        source.open();
        const auto minimumLine = location.line().retreated(unit::LineCount{2U});
        auto currentLine = unit::LineIndex::zero();
        if (const auto cachedPosition = linePositionAtOrBefore(minimumLine);
            cachedPosition.has_value() && source.setStreamPosition(cachedPosition->position, cachedPosition->line)) {
            currentLine = cachedPosition->line;
        }
        const auto maximumLine = location.line().advanced(unit::LineCount{2U});
        while (!source.atEnd() && currentLine <= maximumLine) {
            if (source.readLine().isEmpty()) {
                break;
            }
            ++currentLine;
        }
        return source.TextStreamSource::codeSnippet(location);
    } catch (...) {
        return std::nullopt;
    }
}

void FileSource::rememberLinePosition(const unit::LineIndex line, const unit::ByteIndex position) noexcept {
    try {
        _recentLinePositions.emplace_back(LinePosition{line, position});
        while (_recentLinePositions.size() > 20U) {
            _recentLinePositions.pop_front();
        }
    } catch (...) {
        _recentLinePositions.clear();
    }
}

auto FileSource::linePositionAtOrBefore(const unit::LineIndex line) const noexcept -> std::optional<LinePosition> {
    for (auto iterator = _recentLinePositions.rbegin(); iterator != _recentLinePositions.rend(); ++iterator) {
        if (iterator->line <= line) {
            return *iterator;
        }
    }
    return std::nullopt;
}

auto FileSource::createStream() -> stream::TextInputStreamPtr {
    const auto info = _path.info();
    if (!info.isRegularFile()) {
        throw ConfError(ConfErrorCategory::IO, "The source path is no regular file."_el, Location{_identifier});
    }
    auto options = path::PathReadTextOptions{};
    options.setEncoding(text::StringEncoding::Utf8)
        .setBomMode(text::StringBomMode::Automatic)
        .setEncodingMode(text::EncodingMode::Strict);
    return info.resolvedPath().content().openTextInputStream(options);
}

}
