// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CaptionPlayerApp.hpp"

#include "CaptionBackend.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/ArgumentCount.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringFormat.hpp>

namespace demo {

using namespace el::text::literals;

void CaptionPlayerApp::initialize() {
    info().setApplicationName("Caption Player"_el);
}

void CaptionPlayerApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Play SubRip subtitle tracks together on a shared timeline."_el);
    options->addOption("file"_el)
        .setRequired()
        .setMaximum(el::ArgumentCount::infinite())
        .setHelp("SubRip file to play."_el);
    options->addOption({"--plain"_el, "plain"_el})
        .setType(el::OptionType::Flag)
        .setHelp("Print timestamped cue changes instead of redrawing the display."_el);
}

void CaptionPlayerApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    for (const auto &file : values->getTextList("file"_el)) {
        _tracks.push_back(CaptionTrack::read(el::Path::fromNativeOrThrow(file)));
    }
    _plain = values->getFlag("plain"_el);
    if (!_plain) {
        DemoTerminal::enable(*this);
        _plain = !terminal()->isInteractive();
    }
    installCaptionBackend();
}

void CaptionPlayerApp::installCaptionBackend() {
    const auto protocol = CaptionProtocol::registerWith(eventRegistry());
    auto captions = std::make_unique<CaptionBackend>(protocol, _tracks);
    captions->start(
        el::TimePoint::now(),
        [this](const CueBoundary &boundary) -> void { displayCue(boundary); },
        [this]() -> void { quit(); });
    mainEventLoop().registerBackend(std::move(captions));
}

void CaptionPlayerApp::displayCue(const CueBoundary &boundary) {
    const auto key = std::pair{boundary.track, boundary.cue};
    if (boundary.entering) {
        _active.insert(key);
    } else {
        _active.erase(key);
    }
    if (_plain) {
        printTransition(boundary);
    } else {
        redraw();
    }
}

void CaptionPlayerApp::printTransition(const CueBoundary &boundary) const {
    const auto &track = _tracks.at(boundary.track);
    const auto &cue = track.cues().at(boundary.cue);
    auto change = "end"_el;
    if (boundary.entering) {
        change = "start"_el;
    }
    el::io::printLine(
        el::StringFormat{"{} ms  {}  {} #{}"_el}.build(
            boundary.offset.toMilliseconds().toRawValue(), change, track.name(), cue.number));
    if (boundary.entering) {
        for (const auto &line : cue.lines) {
            el::io::printLine("    "_el, line);
        }
    }
    el::stdOut()->flush();
}

void CaptionPlayerApp::redraw() {
    terminal()->clearScreen();
    for (const auto &[trackIndex, cueIndex] : _active) {
        const auto &track = _tracks[trackIndex];
        terminal()->printLine(track.name());
        for (const auto &line : track.cues()[cueIndex].lines) {
            terminal()->printLine("    "_el, line);
        }
    }
    terminal()->flush();
}

}
