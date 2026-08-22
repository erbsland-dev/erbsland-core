// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostLookupEventEditor.hpp"

#include "HostLookup.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

auto HostLookupEventEditor::onResolved(HostResolvedFn callback) -> network::HostLookupEventEditor & {
    lookup()->_onResolved = std::move(callback);
    return *this;
}

auto HostLookupEventEditor::onError(NetworkErrorFn callback) -> network::HostLookupEventEditor & {
    lookup()->_onError = std::move(callback);
    return *this;
}

auto HostLookupEventEditor::onFinal(NetworkEventFn callback) -> network::HostLookupEventEditor & {
    lookup()->_onFinal = std::move(callback);
    return *this;
}

auto HostLookupEventEditor::lookup() const -> std::shared_ptr<HostLookup> {
    const auto result = std::dynamic_pointer_cast<HostLookup>(source());
    if (result == nullptr) {
        throw err::LogicError{"The event editor source is not a host lookup."_el};
    }
    return result;
}

}
