// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SourceIdentifier.hpp"

#include "impl/constants/Defaults.hpp"
#include "impl/utilities/InternalView.hpp"

#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../unit/CpCount.hpp"

#include <utility>

namespace erbsland::conf {

using namespace text;
using namespace text::literals;

SourceIdentifier::SourceIdentifier(String name, String path, PrivateTag) noexcept :
    _name{std::move(name)}, _path{std::move(path)} {
}

auto SourceIdentifier::create(String name, String path) noexcept -> SourceIdentifierPtr {
    return std::make_shared<SourceIdentifier>(std::move(name), std::move(path), PrivateTag{});
}

auto SourceIdentifier::createForFile(String path) noexcept -> SourceIdentifierPtr {
    return std::make_shared<SourceIdentifier>(
        String{impl::defaults::fileSourceIdentifier}, std::move(path), PrivateTag{});
}

auto SourceIdentifier::createForText() noexcept -> SourceIdentifierPtr {
    return std::make_shared<SourceIdentifier>(String{impl::defaults::textSourceIdentifier}, String{}, PrivateTag{});
}

auto SourceIdentifier::toText() const noexcept -> String {
    const auto safePath = _path.toSafeString(unit::CpCount{200});
    if (_name.isEmpty() || _path.isEmpty()) {
        if (!_name.isEmpty()) {
            return _name;
        }
        auto result = StringEditor{"unknown:"_el};
        result.append(safePath);
        return result;
    }
    auto result = StringEditor{_name};
    result.append(":"_el).append(safePath);
    return result;
}

auto SourceIdentifier::areEqual(const SourceIdentifierPtr &a, const SourceIdentifierPtr &b) noexcept -> bool {
    if (a == nullptr && b == nullptr) {
        return true;
    }
    if (a != nullptr && b != nullptr) {
        return *a == *b;
    }
    return false;
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const SourceIdentifier &object) -> impl::InternalViewPtr {
    auto view = impl::InternalView::create();
    view->setValue(u8"name", object._name);
    view->setValue(u8"path", object._path);
    return view;
}

#endif

}
