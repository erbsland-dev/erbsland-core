// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DocumentBuilder.hpp"

#include "Document.hpp"
#include "Value.hpp"

#include <stdexcept>

namespace erbsland::conf::impl {

using namespace text::literals;

void DocumentBuilder::reset() noexcept {
    _storage.reset();
}

void DocumentBuilder::addSectionMap(const NamePathLike &namePathLike, const Location &location) {
    auto namePath = toNamePath(namePathLike);
    if (namePath.empty()) {
        throw ConfError{ConfErrorCategory::Syntax, "Can not create section with empty name path."_el, location};
    }
    if (namePath.containsIndex()) {
        throw ConfError{
            ConfErrorCategory::Syntax, "Can not create section with an index in the name path."_el, location, namePath};
    }
    auto [parentValue, value] = _storage.resolveForSection(namePath, location);
    if (value != nullptr) {
        // If there is already a value in place, check if it is an intermediate section.
        if (value->type() != ValueType::IntermediateSection) {
            throw ConfError{
                ConfErrorCategory::NameConflict,
                "A section or value with the same name already exists. This is a conflict with the new section."_el,
                location,
                namePath};
        }
        // transform the intermediate section into a regular one.
        value->transform(ValueType::SectionWithNames);
        value->setLocation(location); // update its location.
    } else {
        // there is no existing element
        value = Value::createSectionWithNames();
        value->setName(namePath.back());
        _storage.addChildValue(parentValue, namePath, location, value);
    }
    _storage.updateLastSection(value, namePath);
}

void DocumentBuilder::addSectionList(const NamePathLike &namePathLike, const Location &location) {
    auto namePath = toNamePath(namePathLike);
    if (namePath.empty()) {
        throw ConfError{ConfErrorCategory::Syntax, "Can not create section list with empty name path."_el, location};
    }
    if (namePath.containsIndex()) {
        throw ConfError{
            ConfErrorCategory::Syntax,
            "Can not create section list with an index in the name path."_el,
            location,
            namePath};
    }
    if (namePath.back().isText()) {
        throw ConfError{
            ConfErrorCategory::Syntax, "Can not create section list with a text name."_el, location, namePath};
    }
    auto [parentValue, value] = _storage.resolveForSection(namePath, location);
    if (value != nullptr) {
        // Only an existing list section is accepted.
        if (value->type() != ValueType::SectionList) {
            throw ConfError{
                ConfErrorCategory::NameConflict,
                u8"A section map or value with the same name already exists. This is a conflict with the new section "
                "list."_el,
                location,
                namePath};
        }
        // Add a new element to the existing section list.
        parentValue = value;
        value = Value::createSectionWithNames();
        _storage.addChildValue(parentValue, namePath, location, value);
    } else {
        // there is no existing element, create a new list with one element.
        value = Value::createSectionList();
        value->setName(namePath.back());
        _storage.addChildValue(parentValue, namePath, location, value);
        parentValue = value;
        value = Value::createSectionWithNames();
        _storage.addChildValue(parentValue, namePath, location, value);
    }
    _storage.updateLastSection(value, namePath);
}

void DocumentBuilder::addValue(const NamePathLike &namePathLike, const ValuePtr &value, const Location &location) {
    auto namePath = toNamePath(namePathLike);
    if (value == nullptr) {
        throw err::ParameterError{"value must not be null.", "value"};
    }
    if (value->type().isUndefined()) {
        throw err::LogicError{"Can not add an undefined value."};
    }
    if (value->type().isMap() || value->type() == ValueType::SectionList) {
        throw err::LogicError{"Use the 'addSection...' methods for adding containers."};
    }
    const auto sectionValue = _storage.resolveForValue(namePath, location);
    if (value->name().empty()) {
        value->setName(namePath.back());
    }
    if (sectionValue != nullptr && sectionValue->hasValue(value->name())) {
        throw ConfError{
            ConfErrorCategory::NameConflict, "A value with the same name already exists."_el, location, namePath};
    }
    _storage.addChildValue(sectionValue, namePath, location, value);
}

auto DocumentBuilder::getDocumentAndReset() noexcept -> std::shared_ptr<Document> {
    return _storage.getDocumentAndReset();
}

}
