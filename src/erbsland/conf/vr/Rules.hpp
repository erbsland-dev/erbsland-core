// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"
#include "Rules_fwd.hpp"

#include "../Document.hpp"
#include "../Value.hpp"

#include <vector>

namespace erbsland::conf::vr {

/// A set of validation rules.
/// @tested{VrPublicApiTest VrDocumentModelTest VrVariableNamesTest}
class Rules {
public:
    // defaults
    virtual ~Rules() = default;

public:
    /// Validate a document or document branch against these rules.
    /// Validation of the values also assigns additional meta-data to the values.
    /// Missing values with defaults are added to the validated branch.
    /// @param value The value or document to validate.
    /// @param version The version of the document to validate.
    /// @throws ConfError (Validation) On any validation error.
    virtual void validate(const ValuePtr &value, Integer version) = 0;

public:
    /// Create and validate rules from a rules-definition document.
    /// This reads all rules from the given document and validates the resulting rules definition.
    /// If the document contains any errors, an exception is thrown.
    /// @param document The document to read rules from.
    /// @return The finalized rules definition.
    /// @throws ConfError (Validation) on any error found in the document or rule definition.
    [[nodiscard]] static auto createFromDocument(const DocumentPtr &document) -> RulesPtr;
};

}
