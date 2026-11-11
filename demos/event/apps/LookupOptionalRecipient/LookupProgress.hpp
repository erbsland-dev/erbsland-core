// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Host.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StandardStreams.hpp>

#include <utility>

namespace demo {

using namespace el::text::literals;

/// Optional progress output, independent of the required lookup result.
class LookupProgress final {
public:
    /// Prepare progress output for one server.
    /// @param host The server being resolved.
    explicit LookupProgress(el::Host host) : _host{std::move(host)} {}

public: // output
    /// Announce that addresses are ready.
    /// @param addresses The current result, borrowed only for this call.
    void onResolved(const el::List<el::IpAddress> &addresses) {
        el::io::printLine("Resolved "_el, _host.toString(), " to "_el, addresses.count(), " address(es)."_el);
    }

private:
    el::Host _host; ///< Context owned by the progress reporter.
};

}
