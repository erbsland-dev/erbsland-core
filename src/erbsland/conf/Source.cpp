// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Source.hpp"

#include "impl/source/FileSource.hpp"
#include "impl/source/StringSource.hpp"

namespace erbsland::conf {

auto Source::fromFile(path::Path path) noexcept -> SourcePtr {
    return std::make_shared<impl::FileSource>(std::move(path));
}

auto Source::fromString(text::String text) noexcept -> SourcePtr {
    return std::make_shared<impl::StringSource>(std::move(text));
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const Source &object) -> impl::InternalViewPtr {
    auto view = impl::InternalView::create();
    if (object.identifier()) {
        view->setValue(u8"identifier", internalView(*object.identifier()));
    } else {
        view->setValue(u8"identifier", u8"<none>");
    }
    return view;
}
#endif

}
