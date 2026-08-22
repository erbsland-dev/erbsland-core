// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticFileContent.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../path/PathContent.hpp"
#include "../../../../path/PathReadDataOptions.hpp"
#include "../../../../path/SymlinkMode.hpp"
#include "../../../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

HttpStaticFileContent::HttpStaticFileContent(path::Path path, const unit::ByteLength length) noexcept :
    _path{std::move(path)}, _length{length} {
}

auto HttpStaticFileContent::open() -> stream::ByteInputStreamPtr {
    const auto lock = std::scoped_lock{_mutex};
    if (_opened) {
        throw err::LogicError{"Static file content can only be opened once."_el};
    }
    _opened = true;
    auto options = path::PathReadDataOptions{};
    options.setSymlinkMode(path::SymlinkMode::Skip);
    return _path.content().openByteInputStream(options);
}

}
