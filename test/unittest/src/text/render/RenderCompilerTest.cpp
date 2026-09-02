// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/EnvironmentOptions.hpp>
#include <erbsland/text/render/impl/CompiledBlock.hpp>
#include <erbsland/text/render/impl/CompiledExtends.hpp>
#include <erbsland/text/render/impl/CompiledInclude.hpp>
#include <erbsland/text/render/impl/CompiledLayout.hpp>
#include <erbsland/text/render/impl/Compiler.hpp>
#include <erbsland/text/render/impl/Engine.hpp>
#include <erbsland/text/render/LayoutSource.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(CompiledBlock CompiledExtends CompiledInclude CompiledLayout Compiler)
class RenderCompilerTest final : public el::UnitTest {
    using CompiledBlock = el::text::render::impl::CompiledBlock;
    using CompiledExtends = el::text::render::impl::CompiledExtends;
    using CompiledInclude = el::text::render::impl::CompiledInclude;
    using CompiledLayout = el::text::render::impl::CompiledLayout;
    using Compiler = el::text::render::impl::Compiler;
    using ConstCompiledLayoutPtr = el::text::render::impl::ConstCompiledLayoutPtr;
    using Engine = el::text::render::impl::Engine;

public:
    void testConstantsAndLocations() {
        const auto layout = compile("before\n{{ user.name }}after"_el);

        REQUIRE_EQUAL(layout->constantCount(), el::unit::ItemCount{4U});
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{0U}), "before\n"_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{1U}), "user"_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{2U}), "name"_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{3U}), "after"_el);
        REQUIRE_EQUAL(layout->program().locationAt(el::unit::ByteIndex{2U}).line(), el::unit::LineIndex{1U});
    }

    void testCommentsAndWhitespaceScanning() {
        const auto layout = compile(" A {#- hidden -#} B "_el);

        REQUIRE_EQUAL(layout->constantCount(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{0U}), " A"_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{1U}), "B "_el);
    }

    void testTagCandidateScanningWithCustomDelimiters() {
        auto options = EnvironmentOptions{};
        options.setExpressionDelimiters(Delimiters{"[["_el, "]]"_el})
            .setStatementDelimiters(Delimiters{"<%"_el, "%>"_el})
            .setCommentDelimiters(Delimiters{"(#"_el, "#)"_el});
        const auto layout = compile("[x<x(x raw [[ _root.member2._leaf ]] (# hidden #) tail"_el, options);

        REQUIRE_EQUAL(layout->constantCount(), el::unit::ItemCount{6U});
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{0U}), "[x<x(x raw "_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{1U}), "_root"_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{2U}), "member2"_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{3U}), "_leaf"_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{4U}), " "_el);
        REQUIRE_EQUAL(layout->constant(el::unit::ItemIndex{5U}), " tail"_el);
    }

    void testMalformedAndUnsupportedTags() {
        REQUIRE_THROWS_AS(RenderError, compile("{{ value"_el));
        REQUIRE_THROWS_AS(RenderError, compile("{% if value %}"_el));
        REQUIRE_THROWS_AS(RenderError, compile("{{ value % other }}"_el));
    }

    void testSharedCompiledArtifactIdentity() {
        const auto options = EnvironmentOptions{};
        const auto leaf = Compiler{"leaf"_el, LayoutSource{"leaf"_el, "memory:leaf"_el, "1"_el}, options}.compile();
        const auto base = Compiler{
            "base"_el,
            LayoutSource{"{% block body %}{% include 'leaf' %}base{% endblock %}"_el, "memory:base"_el, "1"_el},
            options,
            {},
            [leaf](const el::text::String &name, bool) -> ConstCompiledLayoutPtr {
                return name == "leaf"_el ? leaf : ConstCompiledLayoutPtr{};
            }}.compile();
        const auto page = Compiler{
            "page"_el,
            LayoutSource{
                "{% extends 'base' %}{% block body %}page{{ super() }}{% endblock %}"_el, "memory:page"_el, "1"_el},
            options,
            {},
            [base](const el::text::String &name, bool) -> ConstCompiledLayoutPtr {
                return name == "base"_el ? base : ConstCompiledLayoutPtr{};
            }}.compile();

        const auto baseBlock = base->block("body"_el);
        const auto pageBlock = page->block("body"_el);
        const auto chain = page->blockChain("body"_el);
        REQUIRE(baseBlock != nullptr);
        REQUIRE(pageBlock != nullptr);
        REQUIRE(chain != nullptr);
        REQUIRE_EQUAL(chain->size(), std::size_t{2U});
        REQUIRE((*chain)[0U].block == pageBlock);
        REQUIRE((*chain)[1U].block == baseBlock);
        REQUIRE(page->extendsDependency() != nullptr);
        REQUIRE(page->extendsDependency()->layout() == base);
        REQUIRE(base->include(0U) == base->includes()[0U]);
        REQUIRE(base->include(0U)->layout() == leaf);
    }

    void testApplicationFilterSnapshot() {
        auto filters = el::text::StringMap<FilterFn>{};
        filters.set("custom"_el, [](const ValueList &) -> Value { return "old"_el; });
        const auto layout =
            Compiler{
                "page"_el,
                LayoutSource{"{{ 'x' | custom }}"_el, "memory:page"_el, "1"_el},
                EnvironmentOptions{},
                filters}
                .compile();
        filters.set("custom"_el, [](const ValueList &) -> Value { return "new"_el; });

        const auto globalContext = Context{};
        const auto localContext = Context{};
        auto engine = Engine{layout, globalContext, localContext};
        REQUIRE_EQUAL(engine.render(), "old"_el);
    }

private:
    [[nodiscard]] static auto compile(el::text::String text, const EnvironmentOptions &options = EnvironmentOptions{})
        -> ConstCompiledLayoutPtr {
        return Compiler{"page"_el, LayoutSource{std::move(text), "memory:page"_el, "1"_el}, options}.compile();
    }
};
