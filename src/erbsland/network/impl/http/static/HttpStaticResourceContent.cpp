// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticResourceContent.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../resource/ResourceError.hpp"
#include "../../../../resource/ResourceErrorCategory.hpp"
#include "../../../../resource/Resources.hpp"
#include "../../../../stream/ByteBlockInputStream.hpp"
#include "../../../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

HttpStaticResourceContent::HttpStaticResourceContent(
    resource::ResourcesConstPtr retainedResources,
    const resource::Resources &resources,
    text::String identifier,
    text::String path,
    const unit::ByteLength length) noexcept :
    _retainedResources{std::move(retainedResources)},
    _resources{&resources},
    _identifier{std::move(identifier)},
    _path{std::move(path)},
    _length{length} {
}

auto HttpStaticResourceContent::open() -> stream::ByteInputStreamPtr {
    const auto lock = std::scoped_lock{_mutex};
    if (_opened) {
        throw err::LogicError{"Static resource content can only be opened once."_el};
    }
    _opened = true;
    auto data = _resources->getData(_identifier, _path);
    if (!data.has_value()) {
        throw resource::ResourceError{
            resource::ResourceErrorCategory::NotFound, "Static resource content is no longer available."_el};
    }
    if (data->length() != _length) {
        throw resource::ResourceError{
            resource::ResourceErrorCategory::InvalidData, "Static resource length does not match its metadata."_el};
    }
    return std::make_shared<stream::ByteBlockInputStream>(std::move(*data));
}

}
