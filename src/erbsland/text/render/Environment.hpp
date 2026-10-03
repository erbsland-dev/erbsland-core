// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Context.hpp"
#include "Environment_fwd.hpp"
#include "EnvironmentOptions.hpp"
#include "Loader_fwd.hpp"
#include "Value.hpp"

#include "../String.hpp"

namespace erbsland::text::render {

/// A text rendering environment with shared compiled layouts.
/// Configure loaders and filters before rendering; setup methods are not thread-safe.
/// Rendering and global-context replacement are thread-safe.
/// @seedoc{/reference/text/documents_and_rendering}
/// @tested{RenderEnvironmentTest RenderConcurrencyTest RenderFilterTest RenderLanguageCompletionTest}
class Environment {
public:
    /// Create a new empty environment.
    /// @param options The syntax, escaping, and execution limits.
    /// @return An environment ready for loader and filter configuration.
    [[nodiscard]] static auto create(EnvironmentOptions options = {}) -> EnvironmentPtr;

    // defaults
    virtual ~Environment() = default;

public: // setup
    /// Add a layout loader to this environment.
    /// Loaders are tested in the order of priority, or in the order of addition if priorities are equal.
    /// @param loader The loader to add. Must not be null.
    /// @param priority The priority of the loader. Higher values have higher priority.
    /// @note Not thread-safe. Must not be called after the first render call.
    virtual void addLayoutLoader(const LoaderPtr &loader, int priority) = 0;
    /// @overload
    void addLayoutLoader(const LoaderPtr &loader) { addLayoutLoader(loader, 0); }
    /// Add a value filter to this environment.
    /// Filter names use ASCII identifiers. Registered filters can be invoked concurrently while rendering.
    /// The callback list contains the piped value at index zero followed by up to two positional arguments.
    /// The callback validates its own accepted list size and value types.
    /// Invalid template arguments should raise RenderError with category Runtime; err::ParseError is a fallback.
    /// The renderer adds missing source context to RenderError and wraps other callback runtime errors with a cause.
    /// @param name The unique filter name.
    /// @param filter The filter callback. Must not be empty.
    /// @note Not thread-safe. Must not be called after the first render call.
    virtual void addFilter(const String &name, FilterFn filter) = 0;
    /// Enable automatic reload of layouts.
    /// Each render checks source origins, opaque revisions, and dependency generations before execution.
    /// These checks serialize loading and compilation; prefer cached rendering outside development.
    /// Once enabled, auto reload cannot be disabled.
    /// @note Not thread-safe. Must not be called after the first render call.
    virtual void enableAutoReload() = 0;

public: // manage and render
    /// Replace the global context snapshot.
    /// The global context is the fallback if a variable is not found in the local context, or if
    /// no local context is provided.
    /// @param context The new global context.
    virtual void setGlobalContext(Context context) = 0;
    /// Renders the given layout using the given context.
    /// @param layout The path to the layout to render. Must not be empty.
    /// @param context The local context to use for rendering; it overrides global names.
    /// @return The rendered layout.
    /// @throws RenderError If loading, layout syntax, rendering, or a callback reports a runtime error.
    [[nodiscard]] virtual auto render(const String &layout, const Context &context) -> String = 0;
    /// @overload
    [[nodiscard]] auto render(const String &layout) -> String { return render(layout, Context{}); }
};

}
