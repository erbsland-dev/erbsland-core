// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LoopState.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::text::render::impl {

using namespace text::literals;

LoopState::LoopState(Value iterable) : _iterable{std::move(iterable)} {
    if (_iterable.isMap()) {
        _mapIterator = _iterable.asMap().begin();
    }
}

auto LoopState::advance() -> bool {
    if (_started) {
        ++_index;
        if (isMap()) {
            ++_mapIterator;
        }
    } else {
        _started = true;
    }
    return _index < _iterable.itemCount().toSizeT();
}

auto LoopState::listValue() const -> Value {
    if (!isList() || !_started || _index >= _iterable.itemCount().toSizeT()) {
        throw err::LogicError{"There is no current list iteration value."_el};
    }
    return _iterable.get(unit::ItemIndex::fromSizeT(_index));
}

auto LoopState::mapKey() const -> String {
    if (!isMap() || !_started || _index >= _iterable.itemCount().toSizeT()) {
        throw err::LogicError{"There is no current map iteration key."_el};
    }
    return _mapIterator->first;
}

auto LoopState::mapValue() const -> Value {
    if (!isMap() || !_started || _index >= _iterable.itemCount().toSizeT()) {
        throw err::LogicError{"There is no current map iteration value."_el};
    }
    return _mapIterator->second;
}

auto LoopState::metadata() const -> Value {
    const auto length = _iterable.itemCount().toSizeT();
    if (!_started || _index >= length) {
        throw err::LogicError{"There is no current iteration metadata."_el};
    }
    auto result = ValueMap{};
    result.set("index"_el, Value{_index + 1U});
    result.set("index0"_el, Value{_index});
    result.set("revindex"_el, Value{length - _index});
    result.set("revindex0"_el, Value{length - _index - 1U});
    result.set("first"_el, Value{_index == 0U});
    result.set("last"_el, Value{_index + 1U == length});
    result.set("length"_el, Value{length});
    return Value{result};
}

}
