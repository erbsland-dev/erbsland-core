// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Document_fwd.hpp"

#include "impl/value/Value.hpp"

#include <map>

namespace erbsland::conf {

/// A configuration document.
class Document : public Value {
public:
    /// The flat map type mapping name paths to constant value pointers.
    /// Maps each name path to the corresponding constant value in the document.
    using FlatValueMap = std::map<NamePath, ConstValuePtr>;

public:
    // defaults
    ~Document() override = default;

public:
    /// Convert the value structure of this document into a flat map of values.
    /// @return A flat map with all sections and values of this document.
    [[nodiscard]] virtual auto toFlatValueMap() const noexcept -> FlatValueMap = 0;
};

}
