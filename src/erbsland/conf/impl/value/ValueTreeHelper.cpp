// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueTreeHelper.hpp"

#include "../../../text/StringEditor.hpp"
#include "../../../text/StringFormat.hpp"

#include <stdexcept>

namespace erbsland::conf::impl {

using namespace text::literals;

auto ValueTreeHelper::createLines() -> text::StringList {
    if (!_lines.isEmpty()) {
        throw err::LogicError("ValueTreeHelper::createLines() called twice.");
    }
    initStack();
    while (!_stack.empty()) {
        const auto [value, indent, isLast] = popFrame();
        const auto name = computeName(value);
        const auto posText = computePosition(value);
        emitLine(value, name, posText, indent, isLast);
        pushChildren(value, indent, isLast);
    }
    if (_format.isSet(TestFormat::ShowSourceIdentifier)) {
        appendSourceLabels();
    }
    return _lines;
}

void ValueTreeHelper::initStack() noexcept {
    _stack.clear();
    _stack.reserve(64);
    _stack.emplace_back(_rootValue, text::String{}, true);
}

auto ValueTreeHelper::popFrame() noexcept -> Frame {
    const auto frame = _stack.back();
    _stack.pop_back();
    return frame;
}

auto ValueTreeHelper::computeName(const conf::ConstValuePtr &value) noexcept -> text::String {
    if (value->isDocument()) {
        return "<Document>"_el;
    }
    if (value->namePath().empty()) {
        return "<Empty>"_el;
    }
    return value->namePath().back().toPathText();
}

auto ValueTreeHelper::computePosition(const conf::ConstValuePtr &value) -> text::String {
    text::StringEditor positionStr;
    if (_format.isSet(TestFormat::ShowPosition) || _format.isSet(TestFormat::ShowSourceIdentifier)) {
        positionStr.append("["_el);
        if (_format.isSet(TestFormat::ShowSourceIdentifier)) {
            appendSourceIdentifier(positionStr, value);
        }
        if (_format.isSet(TestFormat::ShowPosition)) {
            positionStr.append(value->location().codeLocation().toString());
        }
        positionStr.append("]"_el);
    }
    return positionStr;
}

auto ValueTreeHelper::appendSourceIdentifier(text::StringEditor &positionStr, const conf::ConstValuePtr &value)
    -> void {
    if (auto sid = value->location().sourceIdentifier(); sid == nullptr) {
        positionStr.append("no source"_el);
    } else if (const auto labelIt = _labelMap.find(sid); labelIt != _labelMap.end()) {
        positionStr.append(labelIt->second);
    } else {
        static const text::String labels = "ABCDEFGHIJKLMNPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz+"_el;
        text::StringEditor labelEditor;
        labelEditor.append(labels.charAt(unit::CpIndex::fromSizeT(_labelIndex)));
        const text::String label = labelEditor;
        if (_labelIndex < labels.characterLength().toSizeT() - 2) {
            _labelIndex += 1;
        }
        _labelMap.emplace(sid, label);
        _labelList.emplace_back(label, sid);
        positionStr.append(label);
    }
    positionStr.append(":"_el);
}

void ValueTreeHelper::emitLine(
    const conf::ConstValuePtr &value,
    const text::String &name,
    const text::String &pos,
    const text::String &indent,
    const bool isLast) {
    if (value == _rootValue) {
        _lines.append(text::StringFormat{"{} => {}{}"_el}.build(name, value->toTestText(_format), pos));
    } else {
        text::StringEditor line{indent};
        line.append(isLast ? "└───"_el : "├───"_el);
        line.append(text::StringFormat{"{} => {}{}"_el}.build(name, value->toTestText(_format), pos));
        _lines.append(text::String{line});
    }
}

void ValueTreeHelper::pushChildren(const conf::ConstValuePtr &value, const text::String &indent, const bool isLast) {
    const auto count = value->size();
    for (std::size_t i = count; i > 0;) {
        i -= 1;
        const bool last = (i == count - 1);
        const auto child = value->value(i);
        text::StringEditor childIndent{indent};
        if (value != _rootValue) {
            childIndent.append(isLast ? "    "_el : "│   "_el);
        }
        _stack.emplace_back(child, text::String{childIndent}, last);
    }
}

void ValueTreeHelper::appendSourceLabels() {
    for (const auto &[label, sourceIdentifier] : _labelList) {
        _lines.append(text::StringFormat{"{}: {}"_el}.build(label, sourceIdentifier->toText()));
    }
}

}
