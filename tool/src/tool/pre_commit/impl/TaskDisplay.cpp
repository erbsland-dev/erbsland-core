// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TaskDisplay.hpp"

#include "../../frame/ToolApplication.hpp"

#include <erbsland/cterm/BlockString.hpp>
#include <erbsland/cterm/BlockStringEditor.hpp>
#include <erbsland/cterm/TerminalDocumentRenderer.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

#include <algorithm>
#include <array>

namespace tool::pre_commit::impl {

using namespace el::text::literals;
using el::cterm::Foreground;

TaskDisplay::TaskDisplay(el::cterm::TerminalPtr terminal) :
    _terminal{std::move(terminal)}, _originalStyle{_terminal->style()} {
    _interactive =
        _terminal->isInteractive() && _terminal->outputMode() == el::cterm::Terminal::OutputMode::FullControl;
    if (_interactive) {
        _thread = frame::app().createEventThread();
        _timer = _thread->loop()->createTimer([this]() -> void { refresh(); });
        _timer->startFixedDelay(el::time::TimeDelta::milliseconds(100));
        _thread->start();
    }
}

TaskDisplay::~TaskDisplay() {
    if (_timer) {
        _timer->stop();
        _thread->quit();
        _thread->join();
    }
    const auto guard = _terminal->synchronizeOutput();
    if (_active && _interactive) {
        _terminal->writeLineBreak();
    }
    _terminal->setStyle(_originalStyle);
    _terminal->flush();
}

void TaskDisplay::begin(el::String title, frame::TaskProgress progress) {
    const auto lock = std::lock_guard{_mutex};
    _title = std::move(title);
    _progress = std::move(progress);
    _elapsed.restart();
    _tick = 0;
    _active = true;
    const auto guard = _terminal->synchronizeOutput();
    if (_interactive) {
        writeTaskRow("► "_el, "…"_el, Foreground::Cyan);
    } else {
        _terminal->printLine("[RUN] "_el, _title);
        _terminal->flush();
    }
}

auto TaskDisplay::progressText(const frame::TaskProgressSnapshot &snapshot, const int width) -> el::String {
    if (snapshot.total) {
        const auto completed = snapshot.completed.toSizeT();
        const auto total = snapshot.total->toSizeT();
        auto fraction = 1.0;
        if (total != 0) {
            fraction = static_cast<double>(completed) / static_cast<double>(total);
        }
        const auto filled = static_cast<unsigned int>(fraction * 12.0);
        const auto bar = el::join(
            el::String::fromCharacter(U'█', el::CpCount::fromSizeT(filled)),
            el::String::fromCharacter(U'░', el::CpCount::fromSizeT(12U - filled)));
        auto counts =
            el::StringFormat{"{}% {}/{}"_el}.build(static_cast<unsigned int>(fraction * 100.0), completed, total);
        if (el::cterm::BlockString{counts}.displayWidth() + 15 <= width) {
            return el::join("["_el, bar, "] "_el, counts);
        }
        if (el::cterm::BlockString{counts}.displayWidth() > width) {
            counts = el::StringFormat{"{}%"_el}.build(static_cast<unsigned int>(fraction * 100.0));
        }
        return counts;
    }
    static constexpr auto cFrames = std::array{U'◐', U'◓', U'◑', U'◒'};
    const auto frame = el::String::fromCharacter(cFrames[_tick++ % cFrames.size()], el::CpCount{1});
    return el::StringFormat{"{} {:.1f}s"_el}.build(frame, _elapsed.elapsed().toSecondsWithFractions());
}

void TaskDisplay::refresh() {
    const auto lock = std::lock_guard{_mutex};
    if (!_active || !_interactive) {
        return;
    }
    const auto snapshot = _progress.snapshot();
    const auto guard = _terminal->synchronizeOutput();
    _terminal->testScreenSize();
    const auto detail = el::join(progressText(snapshot, detailWidth()), "  "_el, snapshot.phase);
    writeTaskRow("► "_el, detail, Foreground::Cyan);
}

auto TaskDisplay::detailWidth() const -> int {
    const auto width = _terminal->size().width().toRawValue() - 1;
    const auto titleWidth = std::min(el::cterm::BlockString{_title}.displayWidth(), width / 3);
    return std::max(1, width - titleWidth - 4);
}

void TaskDisplay::writeTaskRow(const el::String &mark, const el::String &detail, const Foreground foreground) {
    _terminal->testScreenSize();
    const auto width = std::max(1, _terminal->size().width().toRawValue() - 1);
    const auto right = el::cterm::BlockString{detail}.croppedToDisplayWidth(
        el::block::Coordinate{detailWidth()}, el::geometry::Alignment::Left);
    const auto leftWidth = std::max(1, width - right.displayWidth() - 2);
    const auto left = el::cterm::BlockString{el::join(mark, _title)}.croppedToDisplayWidth(
        el::block::Coordinate{leftWidth}, el::geometry::Alignment::Left);
    const auto gap = std::max(0, width - left.displayWidth() - right.displayWidth());
    auto row = el::cterm::BlockStringEditor{left};
    row.append(
        el::cterm::BlockString{el::String::fromCharacter(U' ', el::CpCount::fromSizeT(static_cast<std::size_t>(gap)))});
    row.append(right);
    writeRow(row, foreground);
}

void TaskDisplay::writeRow(const el::cterm::BlockString &text, const Foreground foreground) {
    _terminal->testScreenSize();
    const auto width = std::max(1, _terminal->size().width().toRawValue() - 1);
    _terminal->moveLeft(_terminal->size().width());
    _terminal->setStyle(_originalStyle);
    _terminal->setForeground(foreground);
    const auto row = text.croppedToDisplayWidth(el::block::Coordinate{width}, el::geometry::Alignment::Left);
    _terminal->write(row);
    _terminal->setStyle(_originalStyle);
    _terminal->print(
        el::String::fromCharacter(U' ', el::CpCount::fromSizeT(static_cast<std::size_t>(width - row.displayWidth()))));
    _terminal->moveLeft(el::block::Coordinate{width});
    _terminal->flush();
}

void TaskDisplay::finish(const bool success, const bool warnings, const el::time::TimeDelta elapsed) {
    const auto lock = std::lock_guard{_mutex};
    const auto guard = _terminal->synchronizeOutput();
    _active = false;
    auto mark = el::String{"✔ "_el};
    auto color = Foreground{Foreground::Green};
    if (!success) {
        mark = "✖ "_el;
        color = Foreground::Red;
    } else if (warnings) {
        color = Foreground::Yellow;
    }
    auto suffix = el::StringFormat{"  {:.1f}s"_el}.build(elapsed.toSecondsWithFractions());
    if (warnings) {
        suffix = el::join(suffix, "  (warnings)"_el);
    }
    if (_interactive) {
        writeTaskRow(mark, suffix.trimmed(), color);
        _terminal->writeLineBreak();
    } else {
        if (success) {
            _terminal->printLine("[OK] "_el, _title, suffix);
        } else {
            _terminal->printLine("[FAIL] "_el, _title, suffix);
        }
    }
    _terminal->flush();
}

void TaskDisplay::message(const el::LogEntry &entry) {
    const auto guard = _terminal->synchronizeOutput();
    _terminal->setStyle(_originalStyle);
    if (entry.level() == el::LogLevel::Warning) {
        _terminal->setForeground(Foreground::Yellow);
    } else if (entry.level() == el::LogLevel::Error) {
        _terminal->setForeground(Foreground::Red);
    }
    _terminal->printParagraph(el::join("  "_el, entry.message()));
    _terminal->setStyle(_originalStyle);
}

void TaskDisplay::diagnostic(const el::Diagnostic &diagnostic) {
    const auto guard = _terminal->synchronizeOutput();
    el::cterm::TerminalDocumentRenderer{}.renderTo(*_terminal, diagnostic.toTextDocument());
    _terminal->setStyle(_originalStyle);
}

void TaskDisplay::summary(const std::size_t passed, const std::size_t failed, const el::time::TimeDelta elapsed) {
    const auto guard = _terminal->synchronizeOutput();
    _terminal->setForeground(Foreground::Green);
    if (failed != 0) {
        _terminal->setForeground(Foreground::Red);
    }
    _terminal->printLine(
        el::StringFormat{"Pre-commit: {} passed, {} failed ({:.1f}s)."_el}.build(
            passed, failed, elapsed.toSecondsWithFractions()));
    _terminal->setStyle(_originalStyle);
    _terminal->flush();
}

}
