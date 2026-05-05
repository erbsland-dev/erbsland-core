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

/// A set of options with its own parsing callbacks.
/// The parser can combine multiple sets for parsing while still invoking each set's callbacks separately.
/// @tested{OptionsFrameworkTest}
class OptionSet : public OptionSetManager {
public:
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
    [[nodiscard]] static auto create() -> OptionSetPtr;
    /// Add an existing option to this set.
    void addOption(OptionPtr option);

public: // implement OptionsManager
    auto addOption(std::initializer_list<text::StringView> names) -> OptionEditor override;
    auto editOption(const text::StringView &name) -> OptionEditor override;

public: // accessors
    /// Get the help text for this set.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the help text for this set.
    void setHelp(OptionHelp help) { _help = std::move(help); }
    /// Get all options in this set.
    [[nodiscard]] auto options() const noexcept -> const std::vector<OptionPtr> & { return _options; }
    /// Get the set flags.
    [[nodiscard]] auto flags() const noexcept -> OptionFlags { return _flags; }
    /// Set the set flags.
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
    [[nodiscard]] auto findOption(const text::StringView &name) const -> OptionPtr;

private:
    OptionHelp _help;                    ///< The help text for the option set.
    std::vector<OptionPtr> _options;     ///< The options in this set.
    OptionFlags _flags;                  ///< The option set flags.
    PreOptionSetParsingFn _preParsingFn; ///< Called before parsing starts.
    PostParsingFn _postParsingFn;        ///< Called after successful parsing.
};

}
