// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::data::xml {
/// Kinds of nodes retained in an XML document.
enum class XmlNodeType : uint8_t {
    Element,               ///< Element with ordered attributes and children.
    Text,                  ///< Character data.
    Comment,               ///< Comment content.
    CData,                 ///< CDATA content.
    ProcessingInstruction, ///< Processing instruction.
    DocType,               ///< Unresolved DOCTYPE declaration.
    Declaration,           ///< XML declaration.
    EntityReference,       ///< Named or numeric entity reference.
};
}
