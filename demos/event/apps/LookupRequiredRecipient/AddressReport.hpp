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

/// Print the result of a server lookup without owning the operation that supplies it.
class AddressReport final {
public:
    /// Prepare the report for a server.
    /// @param host The server whose addresses will be printed.
    explicit AddressReport(el::Host host) : _host{std::move(host)} {}

public: // output
    /// Print every resolved address.
    /// @param addresses The addresses borrowed for this call.
    void onResolved(const el::List<el::IpAddress> &addresses) {
        for (const auto &address : addresses) {
            el::io::printLine("Server address for "_el, _host.toString(), ": "_el, address);
        }
    }

private:
    el::Host _host; ///< Owned report context; no pointer back to the source.
};

}
