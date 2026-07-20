// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringTree.hpp"

#include "Literals.hpp"

#include <memory>

namespace erbsland::text {

using namespace literals;
using impl::StringTreeData;
using impl::StringTreeDataPtr;
using impl::StringTreeEntry;
using impl::StringTreeEntryKind;

StringTree::StringTree() : _data{createData()} {
}

StringTree::StringTree(String title) : _data{createData(std::move(title))} {
}

StringTree::StringTree(StringTreeDataPtr data) : _data{std::move(data)} {
}

auto StringTree::isEmpty() const noexcept -> bool {
    return _data == nullptr || (_data->title.isEmpty() && _data->entries.empty());
}

auto StringTree::title() const noexcept -> String {
    return _data == nullptr ? String{} : _data->title;
}

auto StringTree::setTitle(String title) -> StringTree & {
    ensureUnique();
    _data->title = std::move(title);
    return *this;
}

auto StringTree::append(String text) -> StringTree & {
    ensureUnique();
    _data->entries.push_back(StringTreeEntry::text(std::move(text)));
    return *this;
}

auto StringTree::append(String label, String value) -> StringTree & {
    ensureUnique();
    _data->entries.push_back(StringTreeEntry::labeledValue(std::move(label), std::move(value)));
    return *this;
}

auto StringTree::append(String label, const bool value) -> StringTree & {
    return append(std::move(label), value ? "true"_el : "false"_el);
}

auto StringTree::append(String label, const StringTree &tree) -> StringTree & {
    ensureUnique();
    _data->entries.push_back(StringTreeEntry::tree(std::move(label), tree._data));
    return *this;
}

auto StringTree::append(String label, StringTree &&tree) -> StringTree & {
    ensureUnique();
    _data->entries.push_back(StringTreeEntry::tree(std::move(label), std::move(tree._data)));
    tree._data = createData();
    return *this;
}

auto StringTree::append(const StringTree &tree) -> StringTree & {
    ensureUnique();
    _data->entries.insert(_data->entries.end(), tree._data->entries.begin(), tree._data->entries.end());
    return *this;
}

auto StringTree::toString(const unit::CpLength indentWidth, const unit::CpLength initialIndentWidth) const -> String {
    auto result = StringEditor{};
    auto firstLine = true;
    appendData(result, *_data, indentWidth, initialIndentWidth, firstLine);
    return result;
}

auto StringTree::createData(String title) -> StringTreeDataPtr {
    auto result = std::make_shared<StringTreeData>();
    result->title = std::move(title);
    return result;
}

void StringTree::appendData(
    StringEditor &result,
    const StringTreeData &data,
    const unit::CpLength indentWidth,
    const unit::CpLength indent,
    bool &firstLine) {
    auto entryIndent = indent;
    if (!data.title.isEmpty()) {
        appendLinePrefix(result, indent, firstLine);
        result.append(data.title);
        if (!data.entries.empty()) {
            result.append(U':');
            entryIndent += indentWidth;
        }
    }
    for (const auto &entry : data.entries) {
        appendEntry(result, entry, indentWidth, entryIndent, firstLine);
    }
}

void StringTree::appendEntry(
    StringEditor &result,
    const StringTreeEntry &entry,
    const unit::CpLength indentWidth,
    const unit::CpLength indent,
    bool &firstLine) {
    appendLinePrefix(result, indent, firstLine);
    if (entry.kind == StringTreeEntryKind::Text) {
        result.append(entry.value);
        return;
    }
    result.append(entry.label);
    result.append(U':');
    if (entry.kind == StringTreeEntryKind::Value) {
        result.append(U' ');
        result.append(entry.value);
        return;
    }
    if (entry.treeData == nullptr || entry.treeData->entries.empty()) {
        result.append(U' ');
        result.append("(empty)"_el);
        return;
    }
    appendData(result, *entry.treeData, indentWidth, indent + indentWidth, firstLine);
}

void StringTree::appendLinePrefix(StringEditor &result, const unit::CpLength indent, bool &firstLine) {
    if (!firstLine) {
        result.append(U'\n');
    }
    firstLine = false;
    result.append(U' ', indent);
}

auto StringTree::listIndexLabel(const std::size_t index) -> String {
    return String::fromJoined({"["_el, String::fromInteger(index), "]"_el});
}

void StringTree::ensureUnique() {
    if (_data == nullptr) {
        _data = createData();
        return;
    }
    if (_data.use_count() > 1) {
        _data = std::make_shared<StringTreeData>(*_data);
    }
}

}
