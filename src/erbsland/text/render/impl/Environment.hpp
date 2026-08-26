// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledLayout_fwd.hpp"

#include "../Environment.hpp"
#include "../LayoutSource.hpp"
#include "../RenderError.hpp"
#include "../RenderErrorCategory.hpp"

#include "../../../text/StringMap.hpp"
#include "../../../util/List.hpp"

#include <mutex>
#include <vector>

namespace erbsland::text::render::impl {

/// The implementation of the environment.
class Environment final : public render::Environment {
    using LoaderEntry = std::pair<LoaderPtr, int>;

public:
    /// Create a new environment.
    explicit Environment(EnvironmentOptions options);

    // defaults/deletions
    ~Environment() override = default;
    Environment(const Environment &) = delete;
    Environment(Environment &&) = delete;
    auto operator=(const Environment &) -> Environment & = delete;
    auto operator=(Environment &&) -> Environment & = delete;

public: // implement Environment
    void addLayoutLoader(const LoaderPtr &loader, int priority) override;
    void addFilter(const String &name, FilterFn filter) override;
    void enableAutoReload() override;
    void setGlobalContext(Context context) override;
    [[nodiscard]] auto render(const String &layout, const Context &context) -> String override;

private:
    /// Ensure the environment wasn't initialized yet.
    void ensureNotInitialized();
    /// Validate the environment options.
    void validateOptions();
    /// Trigger the initialization of the environment.
    void triggerInitialize();
    /// Initialize this environment.
    /// Called once before the first use.
    void initialize();
    /// Validate a logical layout name.
    void validateLayoutName(const String &layout) const;
    /// Load and compile the current generation of a layout.
    [[nodiscard]] auto compiledLayout(const String &layout) -> ConstCompiledLayoutPtr;
    /// Load, refresh, and compile one generation inside a dependency graph.
    [[nodiscard]] auto compiledLayout(
        const String &layout, std::vector<String> &stack, StringMap<ConstCompiledLayoutPtr> &memo)
        -> ConstCompiledLayoutPtr;
    /// Resolve one required or ignored-missing include dependency.
    [[nodiscard]] auto resolveInclude(
        const String &layout, bool ignoreMissing, std::vector<String> &stack, StringMap<ConstCompiledLayoutPtr> &memo)
        -> ConstCompiledLayoutPtr;
    /// Load the source using the configured loader order.
    [[nodiscard]] auto loadLayout(const String &layout, const util::List<LoaderEntry> &loaders) const -> LayoutSource;
    /// Create a render error without a source location.
    [[nodiscard]] static auto error(
        RenderErrorCategory category,
        String title,
        String description,
        const String &layout,
        std::exception_ptr cause = {}) -> RenderError;

private:
    const EnvironmentOptions _options;        ///< The options for the environment.
    mutable std::recursive_mutex _mutex;      ///< The mutex protecting setup and runtime state.
    bool _autoReload{false};                  ///< Whether to automatically reload layouts.
    util::List<LoaderEntry> _loaders;         ///< The loaders for layouts.
    StringMap<FilterFn> _applicationFilters;  ///< Immutable application filters after initialization.
    bool _initialized{false};                 ///< Whether the environment is initialized.
    Context _globalContext;                   ///< The global environment context snapshot.
    StringMap<ConstCompiledLayoutPtr> _cache; ///< Current immutable layout generations.
};

}
