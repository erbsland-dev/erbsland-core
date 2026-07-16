// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Argument.hpp"
#include "OperationModifier.hpp"

#include "../engine/Operation.hpp"

#include "../../../text/StdFormatForText.hpp"
#include "../../../text/StringView.hpp"

#include <cstdint>
#include <format>
#include <set>

namespace erbsland::re::impl {

/// One entry in the argument definition.
struct ArgumentDefinition {
    ArgumentKind kind;
    ArgumentType type;
};

/// An entry in the operation data list.
struct OperationData {
    Operation operation;                       ///< The operation.
    Operation baseOperation;                   ///< The base operation.
    text::StringView baseName;                 ///< The base name for the operation.
    std::set<OperationModifier> modifiers;     ///< Modifiers required to the base name to convert the operation.
    text::StringView displayName;              ///< The display name for the operation.
    std::vector<ArgumentDefinition> arguments; /// The arguments and their types.

    /// Get the number of arguments for the operation.
    [[nodiscard]] constexpr auto argumentCount() const noexcept -> std::size_t { return arguments.size(); }
};

/// Access the list of operation data.
[[nodiscard]] auto operationData() noexcept -> const std::vector<OperationData> &;

/// Access the data for a given operation.
/// @param operation The operation.
/// @returns The operation data.
[[nodiscard]] auto dataForOperation(Operation operation) noexcept -> const OperationData &;

/// Get the base operation for the given string.
/// @param baseName The case-insensitive string to search.
/// @return The operation.
/// @throws err::ParameterError If the given string does not match any operation.
[[nodiscard]] auto baseOperationForString(const text::StringView &baseName) -> Operation;

/// Get the modified operation.
/// @param baseOperation The base operation.
/// @param operationModifiers The operation modifiers.
/// @return The modified operation.
/// @throws err::ParameterError If there is no matching operation.
[[nodiscard]] auto modifiedOperation(Operation baseOperation, const std::set<OperationModifier> &operationModifiers)
    -> Operation;

/// Get the diagnostic name for an operation (empty if unknown).
/// @param operation The operation.
/// @return The name for the operation.
[[nodiscard]] auto toString(Operation operation) noexcept -> text::StringView;

/// Get the base name for an operation.
/// @param operation The operation.
/// @return The name for the operation.
[[nodiscard]] auto toBaseName(Operation operation) noexcept -> text::StringView;

}

template <>
struct std::formatter<erbsland::re::impl::Operation> : std::formatter<erbsland::text::StringView> {
    auto format(const erbsland::re::impl::Operation op, std::format_context &ctx) const {
        return std::formatter<erbsland::text::StringView>::format(erbsland::re::impl::toString(op), ctx);
    }
};
