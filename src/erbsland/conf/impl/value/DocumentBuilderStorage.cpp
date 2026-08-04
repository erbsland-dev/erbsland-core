// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DocumentBuilderStorage.hpp"

#include "Document.hpp"
#include "Value.hpp"
#include "ValueHelper.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void DocumentBuilderStorage::reset() {
    _document = std::make_shared<Document>();
    _lastSectionNamePath = {};
    _lastSectionValue = {};
}

auto DocumentBuilderStorage::getDocumentAndReset() noexcept -> std::shared_ptr<Document> {
    auto result = _document;
    reset();
    return result;
}

void DocumentBuilderStorage::updateLastSection(const ValuePtr &sectionValue, const NamePath &sectionNamePath) {
    if (sectionValue == nullptr) {
        throw err::ParameterError{"sectionValue must not be null."_el, "sectionValue"_el};
    }
    if (sectionNamePath.empty()) {
        throw err::ParameterError{"sectionNamePath must not be empty."_el, "sectionNamePath"_el};
    }
    _lastSectionValue = sectionValue;
    _lastSectionNamePath = sectionNamePath;
}

auto DocumentBuilderStorage::resolveForSection(const NamePath &namePath, const Location &location)
    -> std::pair<ValuePtr, ValuePtr> {

    ValuePtr sectionValue;
    ValuePtr parentValue;
    for (std::size_t i = 0; i < (namePath.size() - 1); ++i) {
        const auto &name = namePath.at(i);
        sectionValue = getChildValue(sectionValue, name);
        if (sectionValue == nullptr) {
            sectionValue = Value::createIntermediateSection();
            sectionValue->setName(name);
            addChildValue(parentValue, namePath, location, sectionValue);
        } else {
            if (sectionValue->type() == ValueType::SectionList) {
                sectionValue = getImplValue(sectionValue->lastValue());
                if (sectionValue == nullptr) {
                    throw err::LogicError{"Empty section list element."};
                }
            } else if (!sectionValue->type().isMap()) {
                throw ConfError{
                    ConfErrorCategory::NameConflict,
                    "One if the intermediate elements in the name path is a value."_el,
                    location,
                    namePath};
            }
        }
        parentValue = sectionValue;
    }
    sectionValue = getChildValue(sectionValue, namePath.back());
    return std::make_pair(parentValue, sectionValue);
}

auto DocumentBuilderStorage::resolveForValue(const NamePath &namePath, const Location &location) -> ValuePtr {
    if (namePath.empty()) {
        throw ConfError{ConfErrorCategory::Syntax, "Can not add a value with an empty name path."_el, location};
    }
    if (namePath.containsIndex()) {
        throw ConfError{
            ConfErrorCategory::Syntax, "Can not add a value with an index in the name path."_el, location, namePath};
    }
    ValuePtr sectionValue;
    if (namePath.size() == 1) {
        sectionValue = _lastSectionValue;
    } else {
        for (std::size_t i = 0; i < (namePath.size() - 1); ++i) {
            const auto &name = namePath.at(i);
            sectionValue = getChildValue(sectionValue, name);
            if (sectionValue == nullptr) {
                throw ConfError{
                    ConfErrorCategory::Syntax, "Can not add a value to a non-existing section."_el, location, namePath};
            }
            if (sectionValue->type() == ValueType::SectionList) {
                sectionValue = getImplValue(sectionValue->lastValue());
                if (sectionValue == nullptr) {
                    throw err::LogicError{"Empty section list element."};
                }
            } else if (!sectionValue->type().isMap()) {
                throw ConfError{
                    ConfErrorCategory::NameConflict,
                    "One if the intermediate elements in the name path is a value."_el,
                    location,
                    namePath};
            }
        }
    }
    return sectionValue;
}

auto DocumentBuilderStorage::getChildValue(const ValuePtr &container, const Name &name) const -> ValuePtr {
    if (container == nullptr) {
        return getImplValue(_document->value(name));
    }
    return getImplValue(container->value(name));
}

void DocumentBuilderStorage::addChildValue(
    const ValuePtr &containerValue, const NamePath &namePath, const Location &location, const ValuePtr &value) {

    validateAddArguments(namePath, value);

    ContainerPtr container;
    conf::ValuePtr newParent;
    if (containerValue == nullptr) {
        container = _document;
        newParent = _document;
        applyRootRules(namePath, location, value);
    } else {
        container = containerValue;
        newParent = containerValue;
        applyContainerRules(containerValue, namePath, location, value);
    }

    value->setParent(newParent);
    value->setLocation(location);
    container->addValue(value);
}

void DocumentBuilderStorage::validateAddArguments(const NamePath &namePath, const ValuePtr &value) const {
    if (namePath.empty()) {
        throw err::ParameterError{"namePath must not be empty."_el, "namePath"_el};
    }
    if (value == nullptr) {
        throw err::ParameterError{"value must not be null."_el, "value"_el};
    }
}

void DocumentBuilderStorage::applyRootRules(const NamePath &namePath, const Location &location, const ValuePtr &value) {

    if (!value->type().isMap() && value->type() != ValueType::SectionList && !value->name().isMeta()) {
        throw ConfError{ConfErrorCategory::Syntax, "Can not add a value to the document root."_el, location, namePath};
    }
    if (value->name().isText()) {
        throw ConfError{
            ConfErrorCategory::NameConflict,
            "Can not add a section with a text name to a document."_el,
            location,
            namePath};
    }
}

void DocumentBuilderStorage::applyContainerRules(
    const ValuePtr &container, const NamePath &namePath, const Location &location, const ValuePtr &value) {

    if (container->name().isText() && value->type().isMap()) {
        throw ConfError{
            ConfErrorCategory::Syntax, "A section with a text name must not have sub sections."_el, location, namePath};
    }

    if (container->type() == ValueType::IntermediateSection) {
        if (!(value->type().isMap() || value->type() == ValueType::SectionList)) {
            throw ConfError{
                ConfErrorCategory::Syntax, "Can not add a value without an existing section."_el, location, namePath};
        }
        if (value->name().isText()) {
            if (!container->empty()) {
                throw ConfError{
                    ConfErrorCategory::NameConflict,
                    "Can not mix sections with text-names and regular names."_el,
                    location,
                    namePath};
            }
            container->transform(ValueType::SectionWithTexts);
            container->setLocation(location);
        }
    } else if (container->type() == ValueType::SectionWithNames) {
        if (value->name().type() == NameType::Text) {
            if (container->empty()) {
                container->transform(ValueType::SectionWithTexts);
            } else {
                throw ConfError{
                    ConfErrorCategory::NameConflict,
                    "Values and sections with text-names can not be mixed with regular names."_el,
                    location,
                    namePath};
            }
        }
    } else if (container->type() == ValueType::SectionWithTexts) {
        if (value->name().type() == NameType::Regular) {
            throw ConfError{
                ConfErrorCategory::NameConflict,
                "Values and sections with text-names can not be mixed with regular names."_el,
                location,
                namePath};
        }
    } else if (container->type() == ValueType::SectionList) {
        if (value->type() != ValueType::SectionWithNames) {
            throw err::LogicError{"Can only add SectionWithNames to SectionList."};
        }
    } else {
        throw err::LogicError{"Values can only added to containers."};
    }
}

}
