// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Environment.hpp"

#include "CompiledExtends.hpp"
#include "CompiledInclude.hpp"
#include "CompiledLayout.hpp"
#include "Compiler.hpp"
#include "Engine.hpp"
#include "ProgramError.hpp"

#include "../Loader.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../text/Char.hpp"
#include "../../StringFormat.hpp"

#include <algorithm>

namespace erbsland::text::render::impl {

using namespace text::literals;

Environment::Environment(EnvironmentOptions options) : _options{std::move(options)} {
    validateOptions();
}

void Environment::addLayoutLoader(const LoaderPtr &loader, int priority) {
    std::unique_lock lock{_mutex};
    ensureNotInitialized();
    if (loader == nullptr) {
        throw err::ParameterError{"The layout loader must not be null."_el, "loader"_el};
    }
    _loaders.forEach([&](const LoaderEntry &entry) -> void {
        if (entry.first == loader) {
            throw err::ParameterError("Loader already added"_el, "loader"_el);
        }
    });
    auto insertAt = unit::ItemIndex::fromSizeT(_loaders.count().toSizeT());
    _loaders.forEach([&](const LoaderEntry &entry, const unit::ItemIndex index) -> util::LoopStatus {
        if (priority > entry.second) {
            insertAt = index;
            return util::LoopStatus::Stop;
        }
        return util::LoopStatus::Continue;
    });
    _loaders.insert(insertAt, std::make_pair(loader, priority));
}

void Environment::addFilter(const String &name, FilterFn filter) {
    std::unique_lock lock{_mutex};
    ensureNotInitialized();
    if (name.isEmpty() ||
        !(name.charAt(unit::ByteIndex::zero()).isAsciiLetter() || name.charAt(unit::ByteIndex::zero()) == U'_')) {
        throw err::ParameterError{"A layout filter name must be an ASCII identifier."_el, "name"_el};
    }
    auto position = unit::ByteIndex::zero();
    const auto end = unit::ByteIndex::end(name.length());
    while (position < end) {
        if (!name.charAt(position).isAsciiWord()) {
            throw err::ParameterError{"A layout filter name must be an ASCII identifier."_el, "name"_el};
        }
        name.advance(position);
    }
    if (!filter) {
        throw err::ParameterError{"A layout filter callback must not be empty."_el, "filter"_el};
    }
    if (name == "escape"_el || name == "e"_el || name == "safe"_el) {
        throw err::ParameterError{"The filter name is reserved for an output modifier."_el, "name"_el};
    }
    if (_applicationFilters.contains(name)) {
        throw err::ParameterError{"A layout filter with this name is already registered."_el, "name"_el};
    }
    _applicationFilters.set(name, std::move(filter));
}

void Environment::enableAutoReload() {
    std::unique_lock lock{_mutex};
    ensureNotInitialized();
    _autoReload = true;
}

void Environment::setGlobalContext(Context context) {
    // Replacing the global snapshot does not trigger initialization.
    std::unique_lock lock{_mutex};
    _globalContext = std::move(context);
}

auto Environment::render(const String &layout, const Context &context) -> String {
    validateLayoutName(layout);
    triggerInitialize();
    const auto generation = compiledLayout(layout);
    auto globalContext = Context{};
    {
        std::unique_lock lock{_mutex};
        globalContext = _globalContext;
    }
    try {
        return Engine{generation, globalContext, context, _options.renderLimits()}.render();
    } catch (const RenderError &) {
        throw;
    } catch (const ProgramError &exception) {
        throw error(
            RenderErrorCategory::Internal,
            "Invalid compiled layout program"_el,
            exception.reason(),
            layout,
            std::current_exception());
    } catch (...) {
        throw error(
            RenderErrorCategory::Runtime,
            "Layout rendering failed"_el,
            "A value callback failed while rendering the layout."_el,
            layout,
            std::current_exception());
    }
}

void Environment::ensureNotInitialized() {
    if (_initialized) {
        throw err::LogicError("Environment is already initialized"_el);
    }
}

void Environment::validateOptions() {
    using DelimiterMemberFn = const Delimiters &(EnvironmentOptions::*)() const noexcept;
    using DelimiterSideFn = const String &(Delimiters::*)() const noexcept;
    using DelimiterEntry = std::pair<DelimiterMemberFn, StringLiteral>;
    constexpr auto delimiterEntries = std::array<DelimiterEntry, 3>{
        DelimiterEntry(&EnvironmentOptions::expressionDelimiters, "expression"_el),
        DelimiterEntry(&EnvironmentOptions::statementDelimiters, "statement"_el),
        DelimiterEntry(&EnvironmentOptions::commentDelimiters, "comment"_el),
    };
    for (const auto &[memberFn, name] : delimiterEntries) {
        const auto &delimiters = (_options.*memberFn)();
        constexpr auto delimiterSide = std::array<std::pair<DelimiterSideFn, StringLiteral>, 2>{
            std::make_pair(&Delimiters::begin, "begin"_el),
            std::make_pair(&Delimiters::end, "end"_el),
        };
        for (const auto &[sideFn, sideName] : delimiterSide) {
            const auto &delimiter = (delimiters.*sideFn)();
            if (delimiter.isEmpty()) {
                throw err::ParameterError(
                    StringFormat{"The {} {} delimiter must not be empty"_el}.build(sideName, name), "options"_el);
            }
            if (delimiter.characterLength() > unit::CpLength{8}) {
                throw err::ParameterError(
                    StringFormat{"The {} {} delimiter must not exceed eight code points"_el}.build(sideName, name),
                    "options"_el);
            }
            if (!delimiter.isValidUtf8()) {
                throw err::ParameterError(
                    StringFormat{"The {} {} delimiter must be valid UTF-8"_el}.build(sideName, name), "options"_el);
            }
        }
        if (delimiters.line().characterLength() > unit::CpLength{8}) {
            throw err::ParameterError(
                StringFormat{"The line {} delimiter must not exceed eight code points"_el}.build(name), "options"_el);
        }
    }
    const auto begins = std::array{
        _options.expressionDelimiters().begin(),
        _options.statementDelimiters().begin(),
        _options.commentDelimiters().begin(),
    };
    for (auto first = std::size_t{0U}; first < begins.size(); ++first) {
        for (auto second = first + 1U; second < begins.size(); ++second) {
            if (begins[first].startsWith(begins[second]) || begins[second].startsWith(begins[first])) {
                throw err::ParameterError{
                    "Layout tag opening delimiters must be distinct and must not be prefixes of each other."_el,
                    "options"_el};
            }
        }
    }
}

void Environment::triggerInitialize() {
    std::unique_lock lock{_mutex};
    if (_initialized) {
        return;
    }
    initialize();
    _initialized = true;
}

void Environment::initialize() {
    if (_loaders.isEmpty()) {
        throw err::LogicError{"At least one layout loader must be configured before rendering."_el};
    }
}

void Environment::validateLayoutName(const String &layout) const {
    if (layout.isEmpty() || layout.characterLength() > unit::CpLength{200}) {
        throw error(
            RenderErrorCategory::InvalidLayoutName,
            "Invalid layout name"_el,
            "A layout name must contain between one and 200 ASCII characters."_el,
            layout);
    }
    auto componentBegin = unit::ByteIndex::zero();
    auto position = unit::ByteIndex::zero();
    const auto end = unit::ByteIndex::end(layout.length());
    while (position < end) {
        const auto character = layout.charAt(position);
        const auto allowed = character.isAsciiLowercaseLetter() || character.isAsciiDigit() || character == U'-' ||
            character == U'_' || character == U'.' || character == U'/';
        if (!allowed) {
            throw error(
                RenderErrorCategory::InvalidLayoutName,
                "Invalid layout name"_el,
                "Layout names accept only lowercase ASCII letters, digits, '-', '_', '.', and '/'."_el,
                layout);
        }
        if (character == U'/') {
            const auto component = layout.slice(unit::ByteRange{componentBegin, position});
            if (component.isEmpty() || component == "."_el || component == ".."_el) {
                throw error(
                    RenderErrorCategory::InvalidLayoutName,
                    "Invalid layout name"_el,
                    "Layout names must not contain empty, '.' or '..' path components."_el,
                    layout);
            }
            layout.advance(position);
            componentBegin = position;
            continue;
        }
        layout.advance(position);
    }
    const auto lastComponent = layout.slice(unit::ByteRange{componentBegin, end});
    if (lastComponent.isEmpty() || lastComponent == "."_el || lastComponent == ".."_el) {
        throw error(
            RenderErrorCategory::InvalidLayoutName,
            "Invalid layout name"_el,
            "Layout names must not end with an empty, '.' or '..' path component."_el,
            layout);
    }
}

auto Environment::compiledLayout(const String &layout) -> ConstCompiledLayoutPtr {
    // Loading and publication are serialized so an older concurrently loaded revision can never replace a newer one.
    // The lock is released before program execution; active renders retain their immutable generation independently.
    std::unique_lock lock{_mutex};
    auto stack = std::vector<String>{};
    auto memo = StringMap<ConstCompiledLayoutPtr>{};
    return compiledLayout(layout, stack, memo);
}

auto Environment::compiledLayout(
    const String &layout, std::vector<String> &stack, StringMap<ConstCompiledLayoutPtr> &memo)
    -> ConstCompiledLayoutPtr {
    if (const auto memoized = memo.get(layout); memoized.has_value()) {
        return *memoized;
    }
    validateLayoutName(layout);
    if (stack.size() > _options.renderLimits().staticDependencyDepth()) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout dependency depth exceeded"_el,
            "A compiled layout graph exceeded the configured static dependency depth."_el,
            layout);
    }
    if (std::ranges::find(stack, layout) != stack.end()) {
        throw error(
            RenderErrorCategory::Syntax,
            "Layout dependency cycle"_el,
            "The static include and inheritance graph contains a direct or indirect cycle."_el,
            layout);
    }
    stack.emplace_back(layout);
    try {
        const auto cached = _cache.get(layout, ConstCompiledLayoutPtr{});
        if (cached != nullptr && !_autoReload) {
            memo.set(layout, cached);
            stack.pop_back();
            return cached;
        }

        auto source = loadLayout(layout, _loaders);
        if (cached != nullptr && cached->source().origin() == source.origin() &&
            cached->source().revision() == source.revision()) {
            auto dependenciesChanged = false;
            if (cached->extendsDependency() != nullptr) {
                const auto &dependency = *cached->extendsDependency();
                try {
                    const auto refreshed = compiledLayout(dependency.name(), stack, memo);
                    if (refreshed != dependency.layout()) {
                        dependenciesChanged = true;
                    }
                } catch (const RenderError &error) {
                    auto context = error.context();
                    context.addOuterFrame(StringFormat{"{}:{}"_el}.build(layout, dependency.location().toString()));
                    throw RenderError{std::move(context), error.cause()};
                }
            }
            for (const auto &dependencyPtr : cached->includes()) {
                const auto &dependency = *dependencyPtr;
                try {
                    const auto refreshed = resolveInclude(dependency.name(), dependency.ignoreMissing(), stack, memo);
                    if (refreshed != dependency.layout()) {
                        dependenciesChanged = true;
                    }
                } catch (const RenderError &error) {
                    auto context = error.context();
                    context.addOuterFrame(StringFormat{"{}:{}"_el}.build(layout, dependency.location().toString()));
                    throw RenderError{std::move(context), error.cause()};
                }
            }
            if (!dependenciesChanged) {
                memo.set(layout, cached);
                stack.pop_back();
                return cached;
            }
        }

        const auto resolver = [&](const String &name, const bool ignoreMissing) -> ConstCompiledLayoutPtr {
            return resolveInclude(name, ignoreMissing, stack, memo);
        };
        auto compiled = Compiler{layout, std::move(source), _options, _applicationFilters, resolver}.compile();
        _cache.set(layout, compiled);
        memo.set(layout, compiled);
        stack.pop_back();
        return compiled;
    } catch (...) {
        stack.pop_back();
        throw;
    }
}

auto Environment::resolveInclude(
    const String &layout, const bool ignoreMissing, std::vector<String> &stack, StringMap<ConstCompiledLayoutPtr> &memo)
    -> ConstCompiledLayoutPtr {
    try {
        const auto result = compiledLayout(layout, stack, memo);
        if (result == nullptr && !ignoreMissing) {
            throw error(
                RenderErrorCategory::LayoutNotFound,
                "Layout not found"_el,
                "No configured loader contains the requested layout."_el,
                layout);
        }
        return result;
    } catch (const RenderError &error) {
        if (ignoreMissing && error.context().category() == RenderErrorCategory::LayoutNotFound) {
            memo.set(layout, ConstCompiledLayoutPtr{});
            return {};
        }
        throw;
    }
}

auto Environment::loadLayout(const String &layout, const util::List<LoaderEntry> &loaders) const -> LayoutSource {
    try {
        for (const auto &entry : loaders) {
            if (auto source = entry.first->load(layout)) {
                if (source->origin().isEmpty() || source->revision().isEmpty()) {
                    throw error(
                        RenderErrorCategory::Load,
                        "Invalid loaded layout source"_el,
                        "A layout loader returned an empty diagnostic origin or revision token."_el,
                        layout);
                }
                return std::move(*source);
            }
        }
    } catch (const RenderError &) {
        throw;
    } catch (...) {
        throw error(
            RenderErrorCategory::Load,
            "Unable to load layout"_el,
            "A configured layout loader failed."_el,
            layout,
            std::current_exception());
    }
    throw error(
        RenderErrorCategory::LayoutNotFound,
        "Layout not found"_el,
        "No configured loader contains the requested layout."_el,
        layout);
}

auto Environment::error(
    const RenderErrorCategory category,
    String title,
    String description,
    const String &layout,
    const std::exception_ptr cause) -> RenderError {
    return RenderError{RenderErrorContext{category, std::move(title), std::move(description)}.setLayout(layout), cause};
}

}
