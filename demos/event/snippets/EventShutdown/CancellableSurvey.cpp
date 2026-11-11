// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CancellableSurvey.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <array>
#include <utility>

namespace demo {

CancellableSurvey::CancellableSurvey(el::EventLoopPtr loop, el::StopToken stopToken) :
    _loop{std::move(loop)}, _stopToken{std::move(stopToken)} {
}

void CancellableSurvey::advance() {
    if (_completions != 0) {
        return;
    }
    if (_stopToken.isStopRequested()) {
        finish(true);
        return;
    }
    constexpr auto habitats = std::array{"青龙林"_el, "凤凰谷"_el, "麒麟山"_el};
    // One bounded unit of work before returning to dispatch.
    el::io::printLine("Surveyed habitat: "_el, habitats[_processed]);
    ++_processed;
    if (_processed == habitats.size()) {
        finish(false);
        return;
    }
    _loop->invoke([this]() -> void { advance(); });
}

void CancellableSurvey::finish(const bool cancelled) {
    _cancelled = cancelled;
    ++_completions;
    if (cancelled) {
        el::io::printLine("Survey cancelled after "_el, _processed, " habitat."_el);
    } else {
        el::io::printLine("Survey complete."_el);
    }
}

}
