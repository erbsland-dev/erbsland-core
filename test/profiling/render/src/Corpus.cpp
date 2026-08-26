// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Corpus.hpp"

#include "CorpusDirectory.hpp"

#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/text/render/Value.hpp>

#include <memory>

namespace app::render::corpus {

namespace el = erbsland;

using namespace el::text::literals;

auto entries() -> const el::List<CorpusEntry> & {
    static const auto cEntries = el::List<CorpusEntry>{
        CorpusEntry{
            "dashboard"_el,
            "realistic-iteration"_el,
            "dashboard.html"_el,
            el::StringList{"html"_el, "unicode"_el, "for-list"_el, "for-map"_el, "if"_el, "filter"_el},
            {},
            4U,
            false,
            CorpusValidation{
                4800U, 654U, 1285U, 2667500202574758463U, 13491420915698704096U, 4384U, 9489228268888998613U}},
        CorpusEntry{
            "email"_el,
            "realistic-expression"_el,
            "email.txt"_el,
            el::StringList{"text"_el, "unicode"_el, "whitespace"_el, "if"_el, "filter"_el},
            {},
            4U,
            false,
            CorpusValidation{
                2992U, 458U, 769U, 5831009180222269763U, 835930858122881969U, 1448U, 5491243608777561891U}},
        CorpusEntry{
            "report"_el,
            "realistic-iteration"_el,
            "report.md"_el,
            el::StringList{"markdown"_el, "for-list"_el, "for-map"_el, "nested-loop"_el, "set"_el},
            {},
            4U,
            false,
            CorpusValidation{
                3548U, 794U, 1540U, 9883084786318212837U, 15534929717216207513U, 1088U, 7520703755824693574U}},
        CorpusEntry{
            "expression-stress"_el,
            "expression-heavy"_el,
            "expression-stress.html"_el,
            el::StringList{"expression"_el, "comparison"_el, "logic"_el, "filter"_el},
            {},
            16U,
            false,
            CorpusValidation{
                9152U, 2098U, 5595U, 7060003797965342899U, 12611334691353345427U, 1728U, 10007483979588375601U}},
        CorpusEntry{
            "control-stress"_el,
            "control-heavy"_el,
            "control-stress.html"_el,
            el::StringList{"if"_el, "elif"_el, "set"_el, "for-list"_el, "for-map"_el},
            {},
            12U,
            false,
            CorpusValidation{
                6264U, 1609U, 3450U, 3054656811621788320U, 3214249140310824076U, 384U, 12541656869339938796U}},
        CorpusEntry{
            "text-heavy"_el,
            "text-heavy"_el,
            "text-heavy.html"_el,
            el::StringList{"raw-text"_el, "unicode"_el, "comment"_el, "expression"_el},
            {},
            32U,
            false,
            CorpusValidation{
                53440U, 994U, 1559U, 9653151719207956329U, 12815327067272335442U, 48512U, 11845705121368405410U}},
        CorpusEntry{
            "custom-delimiters"_el,
            "delimiter-heavy"_el,
            "custom-delimiters.txt"_el,
            el::StringList{"custom-delimiters"_el, "for-list"_el, "if"_el, "comment"_el},
            {},
            16U,
            true,
            CorpusValidation{
                5520U, 881U, 1686U, 12024527820362028112U, 14239336600442227038U, 2192U, 3107401760888558850U}},
        CorpusEntry{
            "include-site"_el,
            "include-realistic"_el,
            "include-site.html"_el,
            el::StringList{"html"_el, "include"_el, "include-context"_el, "for-list"_el, "filter"_el},
            el::StringList{
                "include-site/header.html"_el,
                "include-site/navigation.html"_el,
                "include-site/service.html"_el,
                "include-site/footer.html"_el},
            8U,
            false,
            CorpusValidation{
                2853U, 363U, 448U, 4816035932887380903U, 12006022427116416557U, 2568U, 5134345804971823797U}},
        CorpusEntry{
            "include-stress"_el,
            "include-stress"_el,
            "include-stress.html"_el,
            el::StringList{"include"_el, "shared-dependency"_el, "nested-include"_el, "filter"_el},
            el::StringList{
                "include-stress/branch-a.html"_el,
                "include-stress/branch-b.html"_el,
                "include-stress/depth-1.html"_el,
                "include-stress/depth-2.html"_el,
                "include-stress/depth-3.html"_el,
                "include-stress/depth-4.html"_el,
                "include-stress/leaf.html"_el},
            16U,
            false,
            CorpusValidation{
                6084U, 637U, 451U, 2775197932387534981U, 13588109432353486125U, 6976U, 7578305959641094128U}},
        CorpusEntry{
            "inheritance-site"_el,
            "inheritance-realistic"_el,
            "inheritance-site.html"_el,
            el::StringList{
                "inheritance"_el, "nested-block"_el, "super"_el, "captured-super"_el, "include"_el, "for-list"_el},
            el::StringList{
                "inheritance-site/base.html"_el,
                "inheritance-site/section.html"_el,
                "inheritance-site/page.html"_el,
                "inheritance-site/footer.html"_el},
            1U,
            false,
            CorpusValidation{
                2140U, 399U, 403U, 8101923645672888555U, 8117103324290818732U, 817U, 13969813899330157534U}},
        CorpusEntry{
            "inheritance-stress"_el,
            "inheritance-stress"_el,
            "inheritance-stress.html"_el,
            el::StringList{
                "inheritance"_el,
                "override-chain"_el,
                "nested-block"_el,
                "super"_el,
                "captured-super"_el,
                "include-inheritance"_el,
                "for-list"_el},
            el::StringList{
                "inheritance-stress/base.html"_el,
                "inheritance-stress/level-1.html"_el,
                "inheritance-stress/level-2.html"_el,
                "inheritance-stress/level-3.html"_el,
                "inheritance-stress/component-base.html"_el,
                "inheritance-stress/component.html"_el},
            1U,
            false,
            CorpusValidation{
                2940U, 634U, 582U, 13264082802962366138U, 14781919240103372300U, 919U, 1578114834846647021U}},
        CorpusEntry{
            "language-completion.html"_el,
            "language-completion"_el,
            "language-completion.html"_el,
            el::StringList{
                "arithmetic"_el,
                "collections"_el,
                "membership"_el,
                "tests"_el,
                "loop-else"_el,
                "built-in-filters"_el,
                "filter-arguments"_el,
                "automatic-escaping"_el,
                "explicit-escaping"_el,
                "inheritance"_el,
                "super"_el,
                "include"_el},
            el::StringList{
                "language-completion/base.html"_el,
                "language-completion/fragment.xml"_el,
                "language-completion/fragment.elcl"_el,
                "language-completion/fragment.md"_el,
                "language-completion/fragment.json"_el},
            1U,
            false,
            CorpusValidation{
                1943U, 577U, 1087U, 2833138436189985586U, 353247347867596009U, 540U, 13791726977524536605U}},
    };
    return cEntries;
}

auto load(const el::String &selection) -> el::List<PreparedLayout> {
    const auto loader = el::text::render::FileSystemLoader::create(el::Path{el::String{cCorpusDirectory}});
    auto result = el::List<PreparedLayout>{};
    for (const auto &entry : entries()) {
        const auto legacy = entry.id != "language-completion.html"_el;
        if (selection != "all"_el && selection != entry.id && !(selection == "legacy"_el && legacy)) {
            continue;
        }
        const auto loaded = loader->load(entry.sourcePath);
        if (!loaded.has_value()) {
            throw el::ApplicationError{"A render profiling corpus source could not be loaded."_el};
        }
        const auto prepareText = [legacy](const el::String &text) -> el::String {
            return legacy ? text.replacedAll("| escape"_el, "| legacy"_el) : text;
        };
        auto parts = el::StringList{};
        parts.reserve(el::ItemCount{entry.repetitions});
        for (auto repetition = std::uint32_t{}; repetition < entry.repetitions; ++repetition) {
            parts.append(prepareText(loaded->text()));
        }
        auto options = el::text::render::EnvironmentOptions{};
        if (legacy) {
            options.setAutomaticEscapingEnabled(false);
        }
        if (entry.customDelimiters) {
            options.setExpressionDelimiters(el::text::render::Delimiters{"[["_el, "]]"_el})
                .setStatementDelimiters(el::text::render::Delimiters{"<%"_el, "%>"_el})
                .setCommentDelimiters(el::text::render::Delimiters{"(#"_el, "#)"_el});
        }
        auto rootSource = el::text::render::LayoutSource{parts.join(), loaded->origin(), loaded->revision()};
        auto sources = el::StringMap<std::shared_ptr<const el::text::render::LayoutSource>>{};
        sources.set(entry.id, std::make_shared<const el::text::render::LayoutSource>(rootSource));
        for (const auto &dependency : entry.dependencies) {
            const auto dependencySource = loader->load(dependency);
            if (!dependencySource.has_value()) {
                throw el::ApplicationError{"A render profiling dependency source could not be loaded."_el};
            }
            sources.set(
                dependency,
                std::make_shared<const el::text::render::LayoutSource>(
                    prepareText(dependencySource->text()), dependencySource->origin(), dependencySource->revision()));
        }

        auto account = el::text::render::ValueMap{};
        account.set("name"_el, "Erbsland"_el)
            .set("trial"_el, false)
            .set("suspended"_el, false)
            .set("active"_el, true)
            .set("disabled"_el, false)
            .set("region"_el, "Zürich"_el)
            .set("days_left"_el, 14);
        auto navigation = el::text::render::ValueList{};
        for (const auto label : {"Overview"_el, "Services"_el, "Reports"_el}) {
            auto item = el::text::render::ValueMap{};
            item.set("label"_el, label)
                .set("url"_el, "/page"_el)
                .set("active"_el, el::String{label} == el::String{"Overview"_el});
            navigation.append(item);
        }
        auto services = el::text::render::ValueList{};
        for (const auto name : {"API"_el, "Storage"_el}) {
            auto metrics = el::text::render::ValueMap{};
            metrics.set("latency"_el, 12).set("errors"_el, 0);
            auto service = el::text::render::ValueMap{};
            service.set("id"_el, name).set("name"_el, name).set("healthy"_el, true).set("metrics"_el, metrics);
            services.append(service);
        }
        auto userProfile = el::text::render::ValueMap{};
        userProfile.set("display_name"_el, "Ada Lovelace"_el);
        auto user = el::text::render::ValueMap{};
        user.set("display_name"_el, " Ada "_el)
            .set("name"_el, " Ada "_el)
            .set("profile"_el, userProfile)
            .set("email"_el, "ada@example.test"_el)
            .set("show_details"_el, true)
            .set("disabled"_el, false)
            .set("active"_el, true)
            .set("suspended"_el, false)
            .set("pending"_el, false)
            .set("invited"_el, false);
        auto invoice = el::text::render::ValueMap{};
        invoice.set("overdue"_el, false)
            .set("due_soon"_el, true)
            .set("number"_el, "INV-42"_el)
            .set("amount"_el, "120 CHF"_el)
            .set("due_date"_el, "2026-09-01"_el);
        auto links = el::text::render::ValueMap{};
        links.set("preferences"_el, "https://example.test/preferences"_el)
            .set("support"_el, "https://example.test/support"_el);
        auto order = el::text::render::ValueMap{};
        order.set("identifier"_el, "A-42"_el).set("total"_el, 120.5).set("currency"_el, "CHF"_el);
        auto owner = el::text::render::ValueMap{};
        owner.set("name"_el, "Ada"_el).set("team"_el, "Operations"_el);
        auto incident = el::text::render::ValueMap{};
        incident.set("number"_el, 42)
            .set("title"_el, "Network delay"_el)
            .set("resolved"_el, false)
            .set("mitigated"_el, true)
            .set("owner"_el, owner)
            .set("generated_at"_el, "2026-08-29"_el);
        auto event = el::text::render::ValueMap{};
        event.set("time"_el, "10:00"_el).set("summary"_el, "Detected"_el).set("details"_el, "Investigating"_el);
        auto events = el::text::render::ValueList{event};
        auto details = el::text::render::ValueMap{};
        details.set("status"_el, "degraded"_el).set("regions"_el, el::text::render::ValueList{"eu"_el, "us"_el});
        auto affected = el::text::render::ValueMap{};
        affected.set("api"_el, details);
        auto labels = el::text::render::ValueMap{};
        labels.set("priority"_el, "high"_el);
        auto product = el::text::render::ValueMap{};
        product.set("name"_el, "Core"_el);
        auto environment = el::text::render::ValueMap{};
        environment.set("name"_el, "production"_el);
        auto release = el::text::render::ValueMap{};
        release.set("revision"_el, "abc123"_el);
        auto primaryItem = el::text::render::ValueMap{};
        primaryItem.set("visible"_el, true).set("pending"_el, false).set("name"_el, "first"_el);
        auto context = el::text::render::Context{};
        context.set("account"_el, account)
            .set("navigation"_el, navigation)
            .set("services"_el, services)
            .set("user"_el, user)
            .set("invoice"_el, invoice)
            .set("links"_el, links)
            .set("order"_el, order)
            .set("selected_identifier"_el, "A-42"_el)
            .set("first"_el, false)
            .set("second"_el, "selected"_el)
            .set("third"_el, true)
            .set("temperature"_el, 21.5)
            .set("counter"_el, 1)
            .set("incident"_el, incident)
            .set("events"_el, events)
            .set("affected_services"_el, affected)
            .set("labels"_el, labels)
            .set("product"_el, product)
            .set("environment"_el, environment)
            .set("release"_el, release)
            .set("primary"_el, true)
            .set("primary_items"_el, el::text::render::ValueList{primaryItem})
            .set("secondary"_el, false)
            .set("secondary_values"_el, el::text::render::ValueMap{})
            .set("company"_el, "Erbsland"_el);
        context.set("benchmark_text"_el, "<&*\""_el);
        result.append(
            PreparedLayout{
                entry.id,
                std::move(rootSource),
                std::move(sources),
                std::move(context),
                std::move(options),
                entry.validation});
    }
    if (result.isEmpty()) {
        throw el::ApplicationError{"The selected render profiling corpus is empty or unknown."_el};
    }
    return result;
}

}
