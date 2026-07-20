// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Location.hpp"

#include "../text/AnyStringBuilder.hpp"
#include "../text/Literals.hpp"

namespace erbsland::conf {

using namespace text::literals;

auto Location::toText() const -> text::String {
    auto result = text::AnyStringBuilder{};
    if (_sourceIdentifier == nullptr) {
        result.append("<unknown>"_el);
    } else {
        result.append(_sourceIdentifier->toText());
    }
    if (!_codeLocation.line().isNoIndex()) {
        result.append(U':').appendInteger(_codeLocation.line().toSizeT() + 1U);
    }
    if (!_codeLocation.column().isNoIndex()) {
        result.append(U':').appendInteger(_codeLocation.column().toSizeT() + 1U);
    }
    return result.takeString();
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const Location &object) -> impl::InternalViewPtr {
    auto result = impl::InternalView::create();
    if (object._sourceIdentifier) {
        result->setValue(u8"sourceIdentifier", *object._sourceIdentifier);
    } else {
        result->setValue(u8"sourceIdentifier", u8"<none>");
    }
    result->setValue(u8"location", object._codeLocation.toString());
    return result;
}
#endif

}
