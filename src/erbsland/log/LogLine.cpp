// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogLine.hpp"

#include "../text/StringEditor.hpp"

#include <utility>

namespace erbsland::log {

LogLine::LogLine(std::vector<LogLineSegment> segments) : _segments{std::move(segments)} {
    auto text = text::StringEditor{};
    for (const auto &segment : _segments) {
        text.append(segment.text);
    }
    _text = text;
}

}
