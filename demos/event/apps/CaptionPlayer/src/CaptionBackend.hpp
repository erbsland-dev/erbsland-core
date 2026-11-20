// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptionProtocol.hpp"
#include "CaptionTrack.hpp"
#include "CueBoundary.hpp"

#include <erbsland/EventBackend.hpp>
#include <erbsland/EventRegistry.hpp>

#include <functional>

namespace demo {

/// Adapts a finite subtitle timeline to an event loop.
class CaptionBackend final : public el::EventBackend {
public:
    /// Use the playback protocol to merge the tracks into one timeline.
    /// @param protocol The identifiers registered by the application.
    /// @param tracks Validated tracks in playback order.
    CaptionBackend(CaptionProtocol protocol, const std::vector<CaptionTrack> &tracks);

public: // playback
    /// Connect delivery and completion before running the loop.
    /// @param start The monotonic time corresponding to subtitle offset zero.
    /// @param onCue Called during dispatch for each boundary.
    /// @param onFinished Called after delivery of the final boundary.
    void start(el::TimePoint start, std::function<void(const CueBoundary &)> onCue, std::function<void()> onFinished);

public: // implement EventBackend
    [[nodiscard]] auto backendId() const noexcept -> el::EventBackendId override;
    void attach(el::EventBackendTargetWeakPtr target, el::EventLoopDriverWeakPtr driver) override;
    void poll(el::TimePoint now) override;
    [[nodiscard]] auto handleEvent(const el::Event &event) -> bool override;
    [[nodiscard]] auto nextWakeTime() const -> std::optional<el::TimePoint> override;

private:
    CaptionProtocol _protocol; ///< Shared runtime identifiers.
    el::EventBackendTargetWeakPtr _target;
    std::vector<CueBoundary> _timeline;
    std::size_t _next{};      ///< Index of the next boundary to post.
    std::size_t _delivered{}; ///< Boundaries consumed by the display.
    el::TimePoint _start;
    std::function<void(const CueBoundary &)> _onCue;
    std::function<void()> _onFinished;
};

}
