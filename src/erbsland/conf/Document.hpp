// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/value/Value.hpp"

#include <map>

namespace erbsland::conf {

class Document;
using DocumentPtr = std::shared_ptr<Document>;

/// A configuration document.
class Document : public Value {
public:
    /// The flat map type mapping name paths to constant value pointers.
    /// Maps each name path to the corresponding constant value in the document.
    using FlatValueMap = std::map<NamePath, ConstValuePtr>;

public:
    /// Default destructor.
    ~Document() override = default;

public:
    /// Convert the value structure of this document into a flat map of values.
    /// @return A flat map with all sections and values of this document.
    [[nodiscard]] virtual auto toFlatValueMap() const noexcept -> FlatValueMap = 0;
};

}
