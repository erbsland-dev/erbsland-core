// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionHelpTarget.hpp"

#include "../Options_fwd.hpp"
#include "../OptionSet_fwd.hpp"

#include <vector>

namespace erbsland::options::impl {

/// Collect and search visible module, option and choice help targets in scope order.
/// @tested{OptionHelpDocumentTest}
class OptionHelpSearch final {
public:
    /// Collect targets from the selected module, globals, then other modules.
    /// @param options Root definitions to search.
    /// @param moduleName Selected module, if any.
    OptionHelpSearch(OptionsPtr options, const text::String &moduleName);

public: // search
    /// Find exact targets for a validated help selector.
    /// @param name A simple or qualified help selector.
    /// @return Distinct exact targets in scope order.
    [[nodiscard]] auto exactMatches(const text::String &name) const -> std::vector<OptionHelpTarget>;
    /// Find fuzzy targets, falling back to prefixes only when no fuzzy target exists.
    /// @param name A simple or qualified help selector.
    /// @return Distinct targets ranked by edit distance, then scope order, or prefix targets in scope order.
    [[nodiscard]] auto partialMatches(const text::String &name) const -> std::vector<OptionHelpTarget>;
    /// Access the deduplicated catalogue.
    [[nodiscard]] auto targets() const noexcept -> const std::vector<OptionHelpTarget> & { return _targets; }
    /// Test whether exact matches must be presented as a list instead of one detail page.
    /// @param matches Exact targets in scope order.
    /// @return Whether different kinds or choices within the same scope need disambiguation.
    [[nodiscard]] static auto requiresResults(const std::vector<OptionHelpTarget> &matches) noexcept -> bool;

private:
    /// Append one visible module and its definitions.
    void appendModule(const OptionModulePtr &module);
    /// Append visible definitions from one set.
    void appendSet(const OptionSetPtr &set, const OptionModulePtr &module);
    /// Append one target unless its identity is already present.
    void appendTarget(OptionHelpTarget target);
    /// Build bare and qualified aliases for a choice.
    [[nodiscard]] static auto choiceNames(const OptionPtr &option, const OptionChoicePtr &choice) -> text::StringList;

private:
    OptionsPtr _options;                    ///< Definitions including enabled built-in requests.
    std::vector<OptionHelpTarget> _targets; ///< Visible targets in lookup order.
};

}
