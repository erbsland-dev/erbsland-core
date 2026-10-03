// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Option_fwd.hpp"
#include "../OptionChoice_fwd.hpp"
#include "../OptionHelp.hpp"
#include "../OptionModule_fwd.hpp"

#include "../../text/StringList.hpp"

namespace erbsland::options::impl {

/// One visible help target, its aliases and its owning scope.
/// @tested{OptionHelpDocumentTest}
struct OptionHelpTarget final {
    OptionModulePtr module; ///< Owning module, or null for global scope.
    OptionPtr option;       ///< Option target or owning option; null for module targets.
    OptionChoicePtr choice; ///< Choice target; null for module and option targets.
    text::StringList names; ///< Normalized exact lookup aliases, including qualified choice aliases.

    /// Test whether this target describes a module.
    [[nodiscard]] auto isModule() const noexcept -> bool { return option == nullptr; }
    /// Test whether this target describes a choice.
    [[nodiscard]] auto isChoice() const noexcept -> bool { return choice != nullptr; }
    /// Access the target's authored help metadata.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp &;
    /// Get an unambiguous selector for generated help commands.
    /// @return The module name, preferred dashed option alias, or qualified choice selector.
    [[nodiscard]] auto commandName() const -> text::String;
    /// Test whether two registrations refer to the same target in the same scope.
    /// @param other Registration to compare.
    /// @return Whether scope, owner and target identities agree.
    [[nodiscard]] auto sameTarget(const OptionHelpTarget &other) const noexcept -> bool;
};

}
