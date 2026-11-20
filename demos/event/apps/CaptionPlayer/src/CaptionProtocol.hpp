// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/EventRegistry.hpp>

namespace demo {

/// The application's identifiers shared by caption backends.
class CaptionProtocol final {
    CaptionProtocol(el::EventBackendId backend, el::EventId cueBoundary) :
        _backend{backend}, _cueBoundary{cueBoundary} {}

public: // accessors
    /// Identifier of the caption backend family.
    [[nodiscard]] auto backend() const noexcept -> el::EventBackendId { return _backend; }
    /// Identifier of a queued cue boundary.
    [[nodiscard]] auto cueBoundary() const noexcept -> el::EventId { return _cueBoundary; }

public: // factories
    /// Register the protocol once during application setup.
    /// @param registry The owning application's event registry.
    /// @return Identifiers to supply to each playback backend.
    static auto registerWith(el::EventRegistry &registry) -> CaptionProtocol {
        using namespace el::text::literals;
        return {
            registry.registerBackend("dev.erbsland.demo.CaptionPlayback"_el),
            registry.registerEvent("dev.erbsland.demo.CueBoundary"_el),
        };
    }

private:
    el::EventBackendId _backend;
    el::EventId _cueBoundary;
};

}
