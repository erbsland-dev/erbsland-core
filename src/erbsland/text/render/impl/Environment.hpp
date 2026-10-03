// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledLayout_fwd.hpp"

#include "../Environment.hpp"
#include "../LayoutSource.hpp"
#include "../RenderError.hpp"
#include "../RenderErrorCategory.hpp"

#include "../../../util/List.hpp"
#include "../../StringList.hpp"
#include "../../StringMap.hpp"

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <vector>

namespace erbsland::text::render::impl {

/// The implementation of the environment.
/// @tested{RenderEnvironmentTest RenderConcurrencyTest RenderIncludeTest RenderInheritanceTest}
class Environment final : public render::Environment {
    using LoaderEntry = std::pair<LoaderPtr, int>;
    /// Immutable inputs retained by one render after releasing the state lock.
    /// @tested{RenderConcurrencyTest}
    struct RenderState {
        ConstCompiledLayoutPtr layout; ///< The compiled layout generation.
        Context globalContext;         ///< The global context snapshot.
    };

public:
    /// Create a new environment.
    /// @param options The immutable syntax and execution options.
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
    /// Initialize once with the compilation lock held.
    void initialize();
    /// Validate a logical layout name.
    void validateLayoutName(const String &layout) const;
    /// Retain a cached generation and global context under a shared state lock, unless reload is enabled.
    [[nodiscard]] auto cachedRenderState(const String &layout) const -> std::optional<RenderState>;
    /// Retain render inputs, serializing initialization and compilation only on a cache miss or reload.
    [[nodiscard]] auto renderState(const String &layout) -> RenderState;
    /// Load, refresh, and compile one generation inside a dependency graph with the compilation lock held.
    [[nodiscard]] auto compiledLayout(const String &layout, StringList &stack, StringMap<ConstCompiledLayoutPtr> &memo)
        -> ConstCompiledLayoutPtr;
    /// Resolve one required or ignored-missing include dependency.
    [[nodiscard]] auto resolveInclude(
        const String &layout, bool ignoreMissing, StringList &stack, StringMap<ConstCompiledLayoutPtr> &memo)
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
    mutable std::shared_mutex _mutex;         ///< Protects cache generations, reload mode, and global context.
    std::recursive_mutex _compileMutex;       ///< Serializes setup and compilation, including loader reentry.
    bool _autoReload{false};                  ///< Whether to automatically reload layouts.
    util::List<LoaderEntry> _loaders;         ///< The loaders for layouts.
    StringMap<FilterFn> _applicationFilters;  ///< Immutable application filters after initialization.
    bool _initialized{false};                 ///< Whether the environment is initialized.
    Context _globalContext;                   ///< The global environment context snapshot.
    StringMap<ConstCompiledLayoutPtr> _cache; ///< Current immutable layout generations.
};

}
