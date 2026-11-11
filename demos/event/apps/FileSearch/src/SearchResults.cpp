// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SearchResults.hpp"

#include <algorithm>

namespace demo {

using namespace el::text::literals;

void SearchResults::reset() {
    ++_generation;
    _paths.clear();
    _selected = {};
    _first = {};
    _status = {};
}

void SearchResults::receive(const std::uint64_t generation, SearchUpdate update) {
    if (generation != _generation) {
        return;
    }
    _paths.append(update.paths);
    if (update.finished) {
        if (update.incomplete) {
            _status = "Incomplete: some paths could not be read."_el;
        } else if (_paths.isEmpty()) {
            _status = "No matches."_el;
        } else {
            _status = "Search complete."_el;
        }
    }
}

void SearchResults::move(const bool down, const el::ItemCount distance) {
    if (_paths.isEmpty()) {
        return;
    }
    if (down) {
        _selected = std::min(_selected + distance, _paths.lastIndex());
    } else {
        _selected -= distance;
    }
    resize(_rows);
}

void SearchResults::resize(const el::ItemCount rows) {
    _rows = std::max(rows, el::ItemCount{1});
    if (_selected < _first) {
        _first = _selected;
    } else if (_selected >= _first + _rows) {
        _first = _selected - (_rows - el::ItemCount{1});
    }
}

auto SearchResults::selectedPath() const -> std::optional<el::Path> {
    if (_paths.isEmpty()) {
        return std::nullopt;
    }
    return _paths.get(_selected);
}

}
