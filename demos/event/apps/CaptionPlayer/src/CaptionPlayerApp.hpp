// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptionTrack.hpp"
#include "CueBoundary.hpp"

#include <erbsland/Application.hpp>

#include <set>

namespace demo {

/// Plays one or more tracks on the same clock and owns the terminal display.
class CaptionPlayerApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;

private:
    void installCaptionBackend();
    void displayCue(const CueBoundary &boundary);
    void printTransition(const CueBoundary &boundary) const;
    void redraw();

private:
    std::vector<CaptionTrack> _tracks;                     ///< Read completely before playback starts.
    std::set<std::pair<std::size_t, std::size_t>> _active; ///< Visible (track, cue) pairs in display order.
    bool _plain{};                                         ///< Print a timeline transcript.
};

}
