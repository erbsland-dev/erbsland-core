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

/// A text rendering environment.
///
/// `layout` is a path like identifier only accepting ASCII lowercase letters, digits, the underscore,
/// minus character and the `/` as path separator `[-._/a-z0-9]`.
/// It must never start or end with `/`, and must not contain two consecutive `/`.
/// The maximum length is 200 code-points.
///
/// Multithreading: The setup methods are not thread-safe. The manage and render methods are thread-safe.
/// @seedoc{/reference/text/render}
/// @tested{RenderEnvironmentTest RenderFilterTest RenderLanguageCompletionTest}
class Environment {
public:
    /// Create a new empty environment.
    [[nodiscard]] static auto create(EnvironmentOptions options = {}) -> EnvironmentPtr;

    // defaults
    virtual ~Environment() = default;

public: // setup
    /// Add a layout loader to this environment.
    /// Loaders are tested in the order of priority, or in the order of addition if priorities are equal.
    /// @param loader The loader to add. Must not be null.
    /// @param priority The priority of the loader. Higher values have higher priority.
    /// @note Not thread-safe. Must not be called after the first manage or render call.
    virtual void addLayoutLoader(const LoaderPtr &loader, int priority) = 0;
    /// @overload
    void addLayoutLoader(const LoaderPtr &loader) { addLayoutLoader(loader, 0); }
    /// Add a value filter to this environment.
    /// Filter names use ASCII identifiers. Registered filters can be invoked concurrently while rendering.
    /// The callback list contains the piped value at index zero followed by up to two positional arguments.
    /// The callback validates its own accepted list size and value types.
    /// @param name The unique filter name.
    /// @param filter The filter callback. Must not be empty.
    /// @note Not thread-safe. Must not be called after the first render call.
    virtual void addFilter(const String &name, FilterFn filter) = 0;
    /// Enable automatic reload of layouts.
    /// This option mainly makes sense while development, as it can impact performance.
    /// Each time a layout is rendered, all loaders are queried and the last modification date/time of the
    /// found layout is compared with the compiled version. If a newer version is found, the layout is
    /// reloaded and compiled.
    /// Once enabled, auto reload cannot be disabled.
    /// @note Not thread-safe. Must not be called after the first manage or render call.
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
    /// @throws RenderError on any failure.
    [[nodiscard]] virtual auto render(const String &layout, const Context &context) -> String = 0;
    /// @overload
    [[nodiscard]] auto render(const String &layout) -> String { return render(layout, Context{}); }
};

}
