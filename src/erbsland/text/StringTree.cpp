// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringTree.hpp"

#include "Literals.hpp"

#include <memory>

namespace erbsland::text {

using namespace literals;

StringTree::StringTree() : _data{createData()} {
}

StringTree::StringTree(StringView title) : _data{createData(std::move(title))} {
}

StringTree::StringTree(impl::StringTreeDataPtr data) : _data{std::move(data)} {
}

auto StringTree::isEmpty() const noexcept -> bool {
    return _data == nullptr || (_data->title.isEmpty() && _data->entries.empty());
}

auto StringTree::title() const noexcept -> StringView {
    return _data == nullptr ? StringView{} : _data->title;
}

auto StringTree::setTitle(StringView title) -> StringTree & {
    ensureUnique();
    _data->title = std::move(title);
    return *this;
}

auto StringTree::append(StringView text) -> StringTree & {
    ensureUnique();
    _data->entries.push_back(impl::StringTreeEntry::text(std::move(text)));
    return *this;
}

auto StringTree::append(StringView label, StringView value) -> StringTree & {
    ensureUnique();
    _data->entries.push_back(impl::StringTreeEntry::labeledValue(std::move(label), std::move(value)));
    return *this;
}

auto StringTree::append(StringView label, const bool value) -> StringTree & {
    return append(std::move(label), value ? "true"_el : "false"_el);
}

auto StringTree::append(StringView label, const StringTree &tree) -> StringTree & {
    ensureUnique();
    _data->entries.push_back(impl::StringTreeEntry::tree(std::move(label), tree._data));
    return *this;
}

auto StringTree::append(StringView label, StringTree &&tree) -> StringTree & {
    ensureUnique();
    _data->entries.push_back(impl::StringTreeEntry::tree(std::move(label), std::move(tree._data)));
    tree._data = createData();
    return *this;
}

auto StringTree::append(const StringTree &tree) -> StringTree & {
    ensureUnique();
    _data->entries.insert(_data->entries.end(), tree._data->entries.begin(), tree._data->entries.end());
    return *this;
}

auto StringTree::toString(const unit::CpLength indentWidth, const unit::CpLength initialIndentWidth) const -> String {
    auto builder = StringBuilder{};
    auto firstLine = true;
    appendData(builder, *_data, indentWidth, initialIndentWidth, firstLine);
    return builder.toU8String();
}

auto StringTree::createData(StringView title) -> impl::StringTreeDataPtr {
    auto result = std::make_shared<impl::StringTreeData>();
    result->title = std::move(title);
    return result;
}

void StringTree::appendData(
    StringBuilder &builder,
    const impl::StringTreeData &data,
    const unit::CpLength indentWidth,
    const unit::CpLength indent,
    bool &firstLine) {
    auto entryIndent = indent;
    if (!data.title.isEmpty()) {
        appendLinePrefix(builder, indent, firstLine);
        builder.append(data.title);
        if (!data.entries.empty()) {
            builder.append(U':');
            entryIndent += indentWidth;
        }
    }
    for (const auto &entry : data.entries) {
        appendEntry(builder, entry, indentWidth, entryIndent, firstLine);
    }
}

void StringTree::appendEntry(
    StringBuilder &builder,
    const impl::StringTreeEntry &entry,
    const unit::CpLength indentWidth,
    const unit::CpLength indent,
    bool &firstLine) {
    appendLinePrefix(builder, indent, firstLine);
    if (entry.kind == impl::StringTreeEntryKind::Text) {
        builder.append(entry.value);
        return;
    }
    builder.append(entry.label);
    builder.append(U':');
    if (entry.kind == impl::StringTreeEntryKind::Value) {
        builder.append(U' ');
        builder.append(entry.value);
        return;
    }
    if (entry.treeData == nullptr || entry.treeData->entries.empty()) {
        builder.append(U' ');
        builder.append("(empty)"_el);
        return;
    }
    appendData(builder, *entry.treeData, indentWidth, indent + indentWidth, firstLine);
}

void StringTree::appendLinePrefix(StringBuilder &builder, const unit::CpLength indent, bool &firstLine) {
    if (!firstLine) {
        builder.append(U'\n');
    }
    firstLine = false;
    builder.append(U' ', indent);
}

auto StringTree::listIndexLabel(const std::size_t index) -> String {
    auto builder = StringBuilder{};
    builder.append(U'[');
    builder.appendInteger(index);
    builder.append(U']');
    return builder.toU8String();
}

void StringTree::ensureUnique() {
    if (_data == nullptr) {
        _data = createData();
        return;
    }
    if (_data.use_count() > 1) {
        _data = std::make_shared<impl::StringTreeData>(*_data);
    }
}

}
