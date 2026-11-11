// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileSearchApp.hpp"

#include "SearchScanner.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/BindEvent.hpp>
#include <erbsland/cterm/Input.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathResolveMode.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/TimeAmounts.hpp>

#include <utility>

namespace demo {

using namespace el::text::literals;
using el::cterm::Key;

void FileSearchApp::initialize() {
    DemoTerminal::enable(*this);
    info().setApplicationName("File Search"_el);
}

void FileSearchApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Find and select a path with an interactive glob search."_el);
    options->addOption(cDirectoryOption)
        .setDefaultValue("."_el)
        .setValueName("directory"_el)
        .setHelpTitle("Choose the search root"_el)
        .setHelpDescription("Defaults to the current directory. Patterns match paths relative to this root."_el);
}

void FileSearchApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    _root = el::Path::fromNativeOrThrow(values->getText(cDirectoryOption)).toAbsoluteOrThrow();
    if (!_root.info().isDirectory()) {
        throw el::OptionError{"Expected a search directory."_el, values->value(cDirectoryOption)};
    }
    if (!terminal()->isInteractive()) {
        throw el::ApplicationError{"File Search requires an interactive terminal."_el};
    }
    loop()->invoke([this]() -> void { begin(); });
}

void FileSearchApp::begin() {
    _worker = createEventThread(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication));
    _worker->start();
    terminal()->input().setMode(el::cterm::Input::Mode::Key);
    _results.setStatus("Type a glob: *.cpp at root, **/*.cpp recursively."_el);
    _debounce = loop()->createTimer([this]() -> void { submitSearch(); });
    _refresh = loop()->createTimer([this]() -> void { tick(); });
    _refresh->startFixedDelay(el::Milliseconds{33});
    tick();
}

void FileSearchApp::tick() {
    if (_exiting) {
        return;
    }
    // Bound input processing so a large paste still leaves time for results and rendering.
    for (auto count = 0; count < 64; ++count) {
        const auto key = terminal()->input().readKey(el::Milliseconds{0});
        if (!key.valid()) {
            break;
        }
        _blink.restart();
        _dirty = true;
        if (key == Key::Escape || key == Key::Enter) {
            requestExit(key == Key::Enter);
            return;
        }
        if (key == Key::Up || key == Key::Down) {
            _results.move(key == Key::Down, el::ItemCount{1});
        } else if (key == Key::PageUp || key == Key::PageDown) {
            _results.move(key == Key::PageDown, _results.rows());
        } else if (_query.edit(key)) {
            queryChanged();
        }
    }
    if (_blink.elapsed() >= el::Milliseconds{1000}) {
        _blink.restart();
    }
    terminal()->testScreenSize();
    const auto size = terminal()->size();
    const auto cursorVisible = _blink.elapsed() < el::Milliseconds{500};
    if (_dirty || size != _lastSize || cursorVisible != _cursorVisible) {
        terminal()->updateScreen(_view.render(_root, _query, _results, size, cursorVisible));
        _dirty = false;
        _lastSize = size;
        _cursorVisible = cursorVisible;
    }
}

void FileSearchApp::queryChanged() {
    _results.reset();
    _generation->store(_results.generation());
    _debounce->stop();
    if (_query.text().isEmpty()) {
        _results.setStatus("Type a glob: *.cpp at root, **/*.cpp recursively."_el);
        return;
    }
    _results.setStatus("Waiting for input…"_el);
    _debounce->startOnce(el::Milliseconds{150});
}

void FileSearchApp::submitSearch() {
    if (_exiting) {
        return;
    }
    _dirty = true;
    // Compile the current input before starting another search.
    auto pattern = el::PathGlobPatternPtr{};
    try {
        pattern = el::PathGlobPattern::compile(_query.text());
    } catch (const el::ParseError &error) {
        // An unfinished glob is ordinary editable input, not an application failure.
        _results.setStatus(el::join("Invalid glob: "_el, error.reason()));
        return;
    }
    _results.setStatus("Searching…"_el);
    const auto generation = _results.generation();
    // Bind on main before crossing to the worker. The reply retains this request's identity.
    auto reply = el::bindEvent([this, generation](SearchUpdate update) -> void {
        if (!_exiting) {
            _results.receive(generation, std::move(update));
            _dirty = true;
        }
    });
    _worker->loop()->invoke(
        [root = _root,
            pattern = std::move(pattern),
            generation,
            token = _generation,
            stopToken = _worker->stopToken(),
            reply = std::move(reply)]() -> void {
            const auto cancelled = [token, generation, stopToken]() -> bool {
                return token->load() != generation || stopToken.isStopRequested();
            };
            SearchScanner{root, pattern}.scan(cancelled, reply);
        });
}

void FileSearchApp::requestExit(const bool select) {
    _exiting = true;
    _generation->fetch_add(1);
    _refresh->stop();
    _debounce->stop();
    // Restore the user's terminal before waiting for an outstanding filesystem call.
    terminal()->restoreScreen();
    if (select) {
        if (const auto path = _results.selectedPath()) {
            el::io::printLine(path->toNativeString());
            el::stream::stdOut()->flush();
        }
    }
    quit();
}

void FileSearchApp::cleanup() noexcept {
    _exiting = true;
    _generation->fetch_add(1);
    if (_refresh) {
        _refresh->stop();
    }
    if (_debounce) {
        _debounce->stop();
    }
    terminal()->restoreScreen();
    // Application has joined the worker before cleanup, including on failure.
    _worker.reset();
}

}
