// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostLookup_fwd.hpp"

#include "../../host_lookup/HostLookupEventEditor.hpp"

#include <memory>
#include <utility>

namespace erbsland::network::impl {

/// Editor for the event handlers of a native host lookup.
/// @tested{HostLookupTest}
class HostLookupEventEditor final : public network::HostLookupEventEditor {
public:
    /// Create an editor for lookup-owned event handlers.
    /// @param source The lookup that owns this editor.
    /// @param target The callback target.
    HostLookupEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        network::HostLookupEventEditor{std::move(source), std::move(target)} {}

public: // implement network::HostLookupEventEditor
    auto onResolved(HostResolvedFn callback) -> network::HostLookupEventEditor & override;
    auto onError(NetworkErrorFn callback) -> network::HostLookupEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> network::HostLookupEventEditor & override;

private:
    /// Lock and return the host lookup that owns this editor.
    [[nodiscard]] auto lookup() const -> std::shared_ptr<HostLookup>;
};

}
