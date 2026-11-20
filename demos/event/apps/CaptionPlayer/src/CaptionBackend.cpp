// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CaptionBackend.hpp"

#include "CueEventData.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/EventBackendTarget.hpp>

#include <algorithm>
#include <tuple>

namespace demo {

using namespace el::text::literals;

CaptionBackend::CaptionBackend(CaptionProtocol protocol, const std::vector<CaptionTrack> &tracks) :
    _protocol{protocol} {
    for (std::size_t track = 0; track < tracks.size(); ++track) {
        for (std::size_t cue = 0; cue < tracks[track].cues().size(); ++cue) {
            const auto &caption = tracks[track].cues()[cue];
            _timeline.push_back({caption.start, true, track, cue});
            _timeline.push_back({caption.end, false, track, cue});
        }
    }
    // At equal offsets, end old cues first, then retain file and cue order.
    std::ranges::sort(_timeline, {}, [](const CueBoundary &item) {
        return std::tie(item.offset, item.entering, item.track, item.cue);
    });
}

void CaptionBackend::start(
    const el::TimePoint start, std::function<void(const CueBoundary &)> onCue, std::function<void()> onFinished) {
    _start = start;
    _onCue = std::move(onCue);
    _onFinished = std::move(onFinished);
}

auto CaptionBackend::backendId() const noexcept -> el::EventBackendId {
    return _protocol.backend();
}

void CaptionBackend::attach(el::EventBackendTargetWeakPtr target, el::EventLoopDriverWeakPtr) {
    _target = std::move(target);
}

auto CaptionBackend::nextWakeTime() const -> std::optional<el::TimePoint> {
    if (_next == _timeline.size()) {
        return {};
    }
    return _start + _timeline[_next].offset;
}

void CaptionBackend::poll(const el::TimePoint now) {
    while (_next < _timeline.size() && _start + _timeline[_next].offset <= now) {
        const auto target = _target.lock();
        auto event = el::Event{_protocol.cueBoundary(), std::make_unique<CueEventData>(_timeline[_next])};
        if (!target || !target->postFromBackend(std::move(event))) {
            throw el::ApplicationError{"Playback stopped because a caption could not be queued."_el};
        }
        ++_next;
    }
}

auto CaptionBackend::handleEvent(const el::Event &event) -> bool {
    if (event.identifier() != _protocol.cueBoundary()) {
        return false;
    }
    const auto *data = dynamic_cast<const CueEventData *>(event.data().get());
    if (!data) {
        throw el::ApplicationError{"A caption event is missing its cue boundary."_el};
    }
    _onCue(data->boundary());
    ++_delivered;
    if (_delivered == _timeline.size()) {
        _onFinished();
    }
    return true;
}

}
