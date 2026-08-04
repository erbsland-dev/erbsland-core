// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionCallback.hpp"
#include "OptionFlag.hpp"
#include "OptionHelp.hpp"
#include "OptionSet_fwd.hpp"
#include "OptionSetManager.hpp"

#include <utility>
#include <vector>

namespace erbsland::options {

/// A set of options with its own help grouping, flags, and parsing callbacks.
/// The parser can combine multiple enabled sets for parsing while still invoking each set's callbacks separately.
/// @tested{OptionsFrameworkTest}
class OptionSet : public OptionSetManager {
public:
    /// Create an empty option set.
    OptionSet() = default;

    // defaults
    ~OptionSet() override = default;
    OptionSet(const OptionSet &) = default;
    auto operator=(const OptionSet &) -> OptionSet & = default;
    OptionSet(OptionSet &&) = default;
    auto operator=(OptionSet &&) -> OptionSet & = default;

public:
    using OptionSetManager::addOption;

    /// Create an empty shared option set.
    /// @return A shared option set with no options, default help visibility, and no callbacks.
    [[nodiscard]] static auto create() -> OptionSetPtr;
    /// Add an existing option to this set.
    /// @param option The option to append. Null pointers are stored as-is and ignored by parser/display code.
    void addOption(OptionPtr option);

public: // implement OptionsManager
    /// Add an option with one or more names.
    auto addOption(std::initializer_list<text::String> names) -> OptionEditor override;
    /// Edit an option selected by one of its names.
    auto editOption(const text::String &name) -> OptionEditor override;

public: // accessors
    /// Get the help metadata for this set.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the complete help metadata for this set.
    /// @param help The replacement help metadata. A non-empty title becomes the help group title.
    void setHelp(OptionHelp help) { _help = std::move(help); }
    /// Set the help title for this set.
    /// @param title Group title used in generated help output.
    void setHelpTitle(text::String title) { _help.setTitle(std::move(title)); }
    /// Set the help description for this set.
    /// @param description Description for renderers that expose set-level help text.
    void setHelpDescription(text::String description) { _help.setDescription(std::move(description)); }
    /// Set the help epilog for this set.
    /// @param epilog Optional trailing text for renderers that expose set-level epilogs.
    void setHelpEpilog(text::String epilog) { _help.setEpilog(std::move(epilog)); }
    /// Set the help visibility for this set.
    /// @param visibility Controls whether options in this set are visible in generated help output.
    void setHelpVisibility(const OptionHelpVisibility visibility) noexcept { _help.setVisibility(visibility); }
    /// Get all options in this set.
    [[nodiscard]] auto options() const noexcept -> const std::vector<OptionPtr> & { return _options; }
    /// Get the set flags.
    [[nodiscard]] auto flags() const noexcept -> OptionFlags { return _flags; }
    /// Set the set flags.
    /// @param flags The replacement flags. `OptionFlag::Disabled` removes the whole set from parsing and help.
    void setFlags(OptionFlags flags) noexcept { _flags = flags; }
    /// Get the pre-parsing callback.
    [[nodiscard]] auto preParsingFn() const noexcept -> const PreOptionSetParsingFn & { return _preParsingFn; }
    /// Set the pre-parsing callback.
    void setPreParsingFn(PreOptionSetParsingFn fn) { _preParsingFn = std::move(fn); }
    /// Get the post-parsing callback.
    [[nodiscard]] auto postParsingFn() const noexcept -> const PostParsingFn & { return _postParsingFn; }
    /// Set the post-parsing callback.
    void setPostParsingFn(PostParsingFn fn) { _postParsingFn = std::move(fn); }

private:
    /// Find an option selected by one of its names.
    [[nodiscard]] auto findOption(const text::String &name) const -> OptionPtr;

private:
    OptionHelp _help;                    ///< The help text for the option set.
    std::vector<OptionPtr> _options;     ///< The options in this set.
    OptionFlags _flags;                  ///< The option set flags.
    PreOptionSetParsingFn _preParsingFn; ///< Called before parsing starts.
    PostParsingFn _postParsingFn;        ///< Called after successful parsing.
};

}
