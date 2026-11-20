// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptionCue.hpp"

#include <erbsland/Path.hpp>

#include <optional>
#include <utility>
#include <vector>

namespace demo {

/// A parsed SubRip file, retained by the player throughout playback.
class CaptionTrack final {
public:
    /// Assemble a named track from its cue records.
    /// @param name Display name for the track.
    /// @param cues Subtitle cues in source order.
    CaptionTrack(el::String name, std::vector<CaptionCue> cues) : _name{std::move(name)}, _cues{std::move(cues)} {}

public: // accessors
    /// The filename shown alongside the captions.
    [[nodiscard]] auto name() const noexcept -> const el::String & { return _name; }
    /// All cues in original file order.
    [[nodiscard]] auto cues() const noexcept -> const std::vector<CaptionCue> & { return _cues; }

public: // factories
    /// Read and validate a complete track before starting playback.
    /// @param path The local SubRip file.
    /// @return The track with all its cue text and timings.
    /// @throws el::RuntimeError If reading or parsing fails.
    static auto read(const el::Path &path) -> CaptionTrack;

private:
    static auto parseTime(const el::String &text) -> std::optional<el::TimeDelta>;

private:
    el::String _name;
    std::vector<CaptionCue> _cues;
};

}
