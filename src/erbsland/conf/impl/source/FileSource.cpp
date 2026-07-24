// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileSource.hpp"

#include "../../../path/PathContent.hpp"
#include "../../../path/PathInfo.hpp"
#include "../../../path/PathReadTextOptions.hpp"
#include "../../../text/StringBomMode.hpp"
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
