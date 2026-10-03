// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/Events.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <memory>
#include <utility>

namespace demo {

/// A recipient that owns its lookup without being retained by the lookup's callbacks.
/// @notest{Compiled documentation demo.}
class IllustrationController final : public std::enable_shared_from_this<IllustrationController> {
public:
    /// Create a controller on the target's owner loop.
    /// @param target The target for lookup callbacks.
    /// @param caption The illustration caption printed with the result.
    IllustrationController(const el::EventsPtr &target, el::String caption) :
        _lookup{target->get<el::Network>().createHostLookup()}, _caption{std::move(caption)} {}

public: // accessors
    /// Access the owned lookup for deliberate independent retention.
    [[nodiscard]] auto lookup() const -> el::HostLookupPtr { return _lookup; }

public: // operations
    /// Start a lookup with a weak reference to this recipient.
    /// Must be called on the owner event loop while the lookup is inactive.
    /// @param host The numeric host used by the demonstration.
    void start(el::Host host) {
        _lookup->events().onResolved([weak = weak_from_this()](const el::List<el::IpAddress> &addresses) -> void {
            if (const auto self = weak.lock(); self != nullptr) {
                el::io::printLine(self->_caption, ": "_el, addresses.first());
            } else {
                el::io::printLine("Recipient was released; result ignored."_el);
            }
        });
        _lookup->start(std::move(host));
    }

private:
    el::HostLookupPtr _lookup; ///< Source owned by the recipient.
    el::String _caption;       ///< State accessed only on the lookup's owner loop.
};

}
