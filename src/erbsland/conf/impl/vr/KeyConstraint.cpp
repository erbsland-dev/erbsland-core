// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

KeyConstraint::KeyConstraint(KeyReferences keyReferences) : _keyReferences{std::move(keyReferences)} {
    setType(vr::ConstraintType::ConfKey);
}

auto KeyConstraint::getKeyReferences() const -> const KeyReferences & {
    return _keyReferences;
}

auto handleKeyConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    if (node->type() != ValueType::Text && node->type() != ValueType::ValueList) {
        throwValidationError("The 'key' value must be a text or a list of text with the referenced keys"_el);
    }
    if (node->type() == ValueType::ValueList) {
        for (const auto &keyReference : node->asValueList()) {
            if (keyReference->type() != ValueType::Text) {
                throwValidationError("The 'key' value must be a text or a list of text with the referenced keys"_el);
            }
        }
    }
    KeyConstraint::KeyReferences keyReferences;
    for (const auto &keyReferenceValue : node->toValueList()) {
        try {
            keyReferences.emplace_back(NamePath::fromText(keyReferenceValue->asText()));
        } catch (const ConfError &error) {
            throwValidationError(
                text::StringFormat{"Invalid name-path for key reference: {}"_el}.build(error.description()),
                keyReferenceValue->namePath(),
                keyReferenceValue->location());
        }
        // the validation of the name-path is done in `RulesDefinitionValidator`.
    }
    return std::make_shared<KeyConstraint>(keyReferences);
}

}
