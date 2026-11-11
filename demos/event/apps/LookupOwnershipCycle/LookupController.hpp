// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/EventTarget.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/HostLookupEventEditor.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/StandardStreams.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Deliberately form a controller/source cycle and provide an explicit way to break it.
class LookupController final : public std::enable_shared_from_this<LookupController> {
public: // operation
    /// Resolve a server and report completion, retaining this controller in its own source's handlers.
    /// Call on the owner loop after constructing the controller in shared ownership.
    /// @param target The owner loop.
    /// @param host The server to resolve.
    /// @param onFinished The application callback that will arrange explicit teardown.
    void start(const el::EventTargetPtr &target, const el::Host &host, std::function<void(el::ExitCode)> onFinished) {
        _lookup = target->get<el::Network>().createHostLookup();
        _lookup->events()
            .onResolved([self = shared_from_this()](const el::List<el::IpAddress> &addresses) -> void {
                self->onResolved(addresses);
            })
            .onError([self = shared_from_this()](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                self->_exitCode = el::ExitCode::failure();
            })
            .onFinal([self = shared_from_this(), onFinished = std::move(onFinished)]() -> void {
                onFinished(self->_exitCode);
            });
        _lookup->start(host);
    }
    /// Release the source and its handlers, including after a failure or deadline.
    /// The caller must retain this controller until the method returns.
    void releaseSource() noexcept { _lookup.reset(); }

private:
    void onResolved(const el::List<el::IpAddress> &addresses) {
        for (const auto &address : addresses) {
            el::io::printLine("Server address: "_el, address);
        }
    }

private:
    el::HostLookupPtr _lookup; ///< Owns handlers that strongly retain this controller: a deliberate cycle.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Operation result.
};

}
